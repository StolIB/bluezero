add_executable(publisher_node publisher_node.cpp)
target_link_libraries(publisher_node b0)

add_executable(publisher_node_c publisher_node_c.c)
target_link_libraries(publisher_node_c b0)

add_executable(publisher_node_object publisher_node_object.cpp)
target_link_libraries(publisher_node_object b0)

add_executable(subscriber_node subscriber_node.cpp)
target_link_libraries(subscriber_node b0)

add_executable(subscriber_node_c subscriber_node_c.c)
target_link_libraries(subscriber_node_c b0)

add_executable(subscriber_node_object subscriber_node_object.cpp)
target_link_libraries(subscriber_node_object b0)

add_executable(subscriber_node_object_manual subscriber_node_object_manual.cpp)
target_link_libraries(subscriber_node_object_manual b0)

add_executable(multi_publisher multi_publisher.cpp)
target_link_libraries(multi_publisher b0)

add_executable(multi_subscriber multi_subscriber.cpp)
target_link_libraries(multi_subscriber b0)

