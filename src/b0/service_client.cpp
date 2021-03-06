#include <b0/service_client.h>
#include <b0/node.h>

#include "resolver.pb.h"
#include "logger.pb.h"

#include <zmq.hpp>

namespace b0
{

ServiceClient::ServiceClient(Node *node, std::string service_name, bool managed, bool notify_graph)
    : Socket(node, ZMQ_REQ, service_name, managed),
      notify_graph_(notify_graph)
{
}

ServiceClient::~ServiceClient()
{
}

void ServiceClient::log(LogLevel level, std::string message) const
{
    boost::format fmt("ServiceClient(%s): %s");
    Socket::log(level, (fmt % name_ % message).str());
}

void ServiceClient::init()
{
    resolve();
    connect();

    if(notify_graph_)
        node_.notifyService(name_, true, true);
}

void ServiceClient::cleanup()
{
    disconnect();

    if(notify_graph_)
        node_.notifyService(name_, true, false);
}

std::string ServiceClient::getServiceName()
{
    return name_;
}

void ServiceClient::call(const std::string &req, std::string &rep)
{
    writeRaw(req);
    readRaw(rep);
}

void ServiceClient::resolve()
{
    if(!remote_addr_.empty())
    {
        log(debug, "Skipping resolution because remote address (%s) was given", remote_addr_);
        return;
    }

    node_.resolveService(name_, remote_addr_);

    log(trace, "Resolved address: %s", remote_addr_);
}

void ServiceClient::connect()
{
    log(trace, "Connecting to %s...", remote_addr_);
    Socket::connect(remote_addr_);
}

void ServiceClient::disconnect()
{
    log(trace, "Disconnecting from %s...", remote_addr_);
    Socket::disconnect(remote_addr_);
}

} // namespace b0

