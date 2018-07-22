#include <b0/publisher.h>
#include <b0/node.h>

#include <zmq.hpp>

namespace b0
{

Publisher::Publisher(Node *node, const std::string &topic_name, bool managed, bool notify_graph)
    : Socket(node, ZMQ_PUB, topic_name, managed),
      notify_graph_(notify_graph)
{
}

Publisher::~Publisher()
{
}

void Publisher::log(LogLevel level, const std::string &message) const
{
    boost::format fmt("Publisher(%s): %s");
    Socket::log(level, (fmt % name_ % message).str());
}

void Publisher::init()
{
    if(remote_addr_.empty())
        remote_addr_ = node_.getXSUBSocketAddress();
    connect();

    if(notify_graph_)
        node_.notifyTopic(name_, false, true);
}

void Publisher::cleanup()
{
    disconnect();

    if(notify_graph_)
        node_.notifyTopic(name_, false, false);
}

std::string Publisher::getTopicName()
{
    return name_;
}

void Publisher::publish(const std::vector<b0::message::MessagePart> &parts)
{
    writeRaw(parts);
}

void Publisher::publish(const std::string &msg, const std::string &type)
{
    writeRaw(msg, type);
}

void Publisher::connect()
{
    log(trace, "Connecting to %s...", remote_addr_);
    Socket::connect(remote_addr_);
}

void Publisher::disconnect()
{
    log(trace, "Disconnecting from %s...", remote_addr_);
    Socket::disconnect(remote_addr_);
}

} // namespace b0

