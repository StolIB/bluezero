#include <b0/node.h>
#include <b0/subscriber.h>

#include <iostream>

/*! \example subscriber_node_object.cpp
 * This is an example of creating a node by subclassing b0::Node.
 * Useful for overriding some node's behavior.
 */

//! \cond HIDDEN_SYMBOLS

class TestSubscriberNode : public b0::Node
{
public:
    TestSubscriberNode(std::string topic)
        : Node("subscriber"),
          sub_(this, topic, &TestSubscriberNode::on)
    {
    }

    void on(const std::string &msg)
    {
        std::cout << "Received: " << msg << std::endl;
    }

private:
    b0::Subscriber sub_;
};

int main(int argc, char **argv)
{
    TestSubscriberNode node(argc > 1 ? argv[1] : "A");
    node.init();
    node.spin();
    node.cleanup();
    return 0;
}

//! \endcond

