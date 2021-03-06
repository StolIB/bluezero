#include <b0/node.h>
#include <b0/config.h>
#include <b0/publisher.h>
#include <b0/subscriber.h>
#include <b0/service_client.h>
#include <b0/service_server.h>
#include <b0/exceptions.h>
#include <b0/logger/logger.h>
#include <b0/utils/thread_name.h>
#include <b0/utils/env.h>
#include <b0/resolver/client.h>

#include <iostream>
#include <cstdlib>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <zmq.hpp>

#include "resolver.pb.h"
#include "logger.pb.h"

namespace b0
{

std::atomic<bool> Node::quit_flag_(false);

bool Node::sigint_handler_setup_ = false;

struct NodePrivate
{
    NodePrivate(Node *node, int io_threads)
        : context_(io_threads)
    {
    }

    zmq::context_t context_;
};

struct NodePrivate2
{
    NodePrivate2(Node *node)
        : resolv_cli_(node)
    {
    }

    resolver::Client resolv_cli_;
};

Node::Node(std::string nodeName)
    : private_(new NodePrivate(this, 1)),
      private2_(new NodePrivate2(this)),
      name_(nodeName),
      state_(NodeState::Created),
      thread_id_(boost::this_thread::get_id()),
      p_logger_(new logger::Logger(this)),
      shutdown_flag_(false)
{
    set_thread_name("main");

    setupSIGINTHandler();
}

Node::~Node()
{
    if(logger::Logger *p_logger = dynamic_cast<logger::Logger*>(p_logger_))
        delete p_logger;
}

void Node::setResolverAddress(const std::string &addr)
{
    // This is mostly needed for implementing the resolver node;
    // but can be used to connect to a different resolver as well.
    // Use this before Node::init().
    resolv_addr_ = addr;
    if(!resolv_addr_.empty()) private2_->resolv_cli_.setRemoteAddress(resolv_addr_);
}

void Node::init()
{
    if(state_ != NodeState::Created)
        throw exception::InvalidStateTransition("init", state_);

    log(debug, "Initialization...");

    private2_->resolv_cli_.init(); // resolv_cli_ is not managed

    announceNode();

    startHeartbeatThread();

    log(debug, "Initializing sockets...");
    for(auto socket : sockets_)
        socket->init();

    state_ = NodeState::Ready;

    log(debug, "Initialization complete.");
}

void Node::shutdown()
{
    if(state_ != NodeState::Ready)
        throw exception::InvalidStateTransition("shutdown", state_);

    log(debug, "Shutting down...");

    shutdown_flag_.store(true);

    log(debug, "Shutting complete.");
}

bool Node::shutdownRequested() const
{
    return shutdown_flag_.load() || quit_flag_.load();
}

void Node::spinOnce()
{
    if(state_ != NodeState::Ready)
        throw exception::InvalidStateTransition("spinOnce", state_);

    // spin sockets:
    for(auto socket : sockets_)
        socket->spinOnce();
}

void Node::spin(double spinRate)
{
    if(state_ != NodeState::Ready)
        throw exception::InvalidStateTransition("spin", state_);

    log(info, "Node spinning...");

    while(!shutdownRequested())
    {
        spinOnce();
        // FIXME: use sleep_until to effectively spin at spinRate Hz...
        // FIXME: ...i.e.: compensate for the time elapsed in spinOnce()
        long usec = 1000000. / spinRate;
        boost::this_thread::sleep_for(boost::chrono::microseconds{usec});
    }

    log(info, "spin() finished");
}

void Node::cleanup()
{
    if(state_ != NodeState::Ready)
        throw exception::InvalidStateTransition("cleanup", state_);

    // stop the heartbeat_thread so that the last zmq socket will be destroyed
    // and we avoid an unclean exit (zmq::error_t: Context was terminated)
    log(debug, "Killing heartbeat thread...");
    heartbeat_thread_.interrupt();
    heartbeat_thread_.join();

    log(debug, "Cleanup sockets...");
    for(auto socket : sockets_)
        socket->cleanup();

    // inform resolver that we are shutting down
    notifyShutdown();

    private2_->resolv_cli_.cleanup(); // resolv_cli_ is not managed

    state_ = NodeState::Terminated;
}

void Node::log(LogLevel level, std::string message) const
{
    if(boost::this_thread::get_id() != thread_id_)
        throw exception::Exception("cannot call Node::log() from another thread");

    p_logger_->log(level, message);
}

void Node::startHeartbeatThread()
{
    log(trace, "Starting heartbeat thread...");
    heartbeat_thread_ = boost::thread(&Node::heartbeatLoop, this);
}

std::string Node::getName() const
{
    // return this node's name, used to address sockets (together with the socket name).
    // we get this value from the resolver node, during the announceNode() phase.
    return name_;
}

NodeState Node::getState() const
{
    return state_;
}

void * Node::getContext()
{
    return &private_->context_;
}

std::string Node::getXPUBSocketAddress() const
{
    return xpub_sock_addr_;
}

std::string Node::getXSUBSocketAddress() const
{
    return xsub_sock_addr_;
}

void Node::addSocket(Socket *socket)
{
    if(state_ != NodeState::Created)
        throw exception::Exception("Cannot create a socket with an already initialized node");

    sockets_.insert(socket);
}

void Node::removeSocket(Socket *socket)
{
    sockets_.erase(socket);
}

std::string Node::hostname()
{
    std::string host_addr = b0::env::get("BWF_HOST_ID");
    if(host_addr != "")
    {
        log(warn, "BWF_HOST_ID variable is deprecated. Use B0_HOST_ID instead.");
        return host_addr;
    }
    host_addr = b0::env::get("B0_HOST_ID");
    if(host_addr != "")
    {
        return host_addr;
    }
    return boost::asio::ip::host_name();
}

int Node::pid()
{
    return ::getpid();
}

std::string Node::threadID()
{
    return boost::lexical_cast<std::string>(thread_id_);
}

int Node::freeTCPPort()
{
    // by binding the OS socket to port 0, an available port number will be used
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 0);
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket(io_service, ep);
    return socket.local_endpoint().port();
}

void Node::notifyTopic(std::string topic_name, bool reverse, bool active)
{
    resolver::Client &resolv_cli_ = private2_->resolv_cli_;
    resolv_cli_.notifyTopic(topic_name, reverse, active);
}

void Node::notifyService(std::string service_name, bool reverse, bool active)
{
    resolver::Client &resolv_cli_ = private2_->resolv_cli_;
    resolv_cli_.notifyService(service_name, reverse, active);
}

void Node::announceService(std::string service_name, std::string addr)
{
    resolver::Client &resolv_cli_ = private2_->resolv_cli_;
    resolv_cli_.announceService(service_name, addr);
}

void Node::resolveService(std::string service_name, std::string &addr)
{
    resolver::Client &resolv_cli_ = private2_->resolv_cli_;
    resolv_cli_.resolveService(service_name, addr);
}

void Node::setAnnounceTimeout(int timeout)
{
    resolver::Client &resolv_cli_ = private2_->resolv_cli_;
    resolv_cli_.setAnnounceTimeout(timeout);
}

std::string Node::freeTCPAddress()
{
    boost::format fmt("tcp://%s:%d");
    return (fmt % hostname() % freeTCPPort()).str();
}

void Node::announceNode()
{
    private2_->resolv_cli_.announceNode(name_, xpub_sock_addr_, xsub_sock_addr_);

    if(logger::Logger *p_logger = dynamic_cast<logger::Logger*>(p_logger_))
        p_logger->connect(xsub_sock_addr_);
}

void Node::notifyShutdown()
{
    private2_->resolv_cli_.notifyShutdown();
}

void Node::heartbeatLoop()
{
    set_thread_name("HB");
    b0::logger::LocalLogger logger(this);
    logger.log(trace, "HB: started");

    while(!shutdownRequested())
    {
        try
        {
            resolver::Client resolv_cli(this);
            resolv_cli.setReadTimeout(1000);
            resolv_cli.init();

            while(!shutdownRequested())
            {
                int64_t time_usec;
                resolv_cli.sendHeartbeat(&time_usec);
                time_sync_.updateTime(time_usec);
                boost::this_thread::sleep_for(boost::chrono::seconds{1});
            }

            resolv_cli.cleanup();
        }
        catch(std::exception &ex)
        {
            logger.log(error, "HB: %s", ex.what());
        }
    }

    logger.log(trace, "HB: finished");
}

int64_t Node::hardwareTimeUSec() const
{
    return time_sync_.hardwareTimeUSec();
}

int64_t Node::timeUSec()
{
    return time_sync_.timeUSec();
}

void Node::setTimesyncMaxSlope(double max_slope)
{
    time_sync_.setMaxSlope(max_slope);
}

void Node::signalHandler(int s)
{
    quit_flag_.store(true);
}

void Node::setupSIGINTHandler()
{
    if(sigint_handler_setup_) return;

#ifdef HAVE_POSIX_SIGNALS
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &Node::signalHandler;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
#endif

    sigint_handler_setup_ = true;
}

} // namespace b0

