#include <b0/subscriber.h>
#include <b0/node.h>

#include "resolver.pb.h"
#include "logger.pb.h"

#include <zmq.hpp>

namespace b0
{

Subscriber::Subscriber(Node *node, std::string topic, boost::function<void(const std::string&)> callback, bool managed, bool notify_graph)
    : Socket(node, ZMQ_SUB, topic, managed),
      notify_graph_(notify_graph),
      callback_(callback)
{
    setHasHeader(true);
}

Subscriber::~Subscriber()
{
}

void Subscriber::log(LogLevel level, std::string message) const
{
    boost::format fmt("Subscriber(%s): %s");
    Socket::log(level, (fmt % name_ % message).str());
}

void Subscriber::init()
{
    if(remote_addr_.empty())
        remote_addr_ = node_.getXPUBSocketAddress();
    connect();

    if(notify_graph_)
        node_.notifyTopic(name_, true, true);
}

void Subscriber::cleanup()
{
    disconnect();

    if(notify_graph_)
        node_.notifyTopic(name_, true, false);
}

void Subscriber::spinOnce()
{
    if(callback_.empty()) return;

    while(poll())
    {
        std::string msg;
        readRaw(msg);
        callback_(msg);
    }
}

std::string Subscriber::getTopicName()
{
    return name_;
}

void Subscriber::connect()
{
    log(trace, "Connecting to %s...", remote_addr_);
    Socket::connect(remote_addr_);
    Socket::setsockopt(ZMQ_SUBSCRIBE, name_.data(), name_.size());
}

void Subscriber::disconnect()
{
    log(trace, "Disconnecting from %s...", remote_addr_);
    Socket::setsockopt(ZMQ_UNSUBSCRIBE, name_.data(), name_.size());
    Socket::disconnect(remote_addr_);
}

} // namespace b0

