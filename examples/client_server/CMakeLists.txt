add_executable(client_node client_node.cpp)
target_link_libraries(client_node b0)

add_executable(client_node_c client_node_c.c)
target_link_libraries(client_node_c b0)

add_executable(client_node_object client_node_object.cpp)
target_link_libraries(client_node_object b0)

add_executable(server_node server_node.cpp)
target_link_libraries(server_node b0)

add_executable(server_node_c server_node_c.c)
target_link_libraries(server_node_c b0)

add_executable(server_node_object server_node_object.cpp)
target_link_libraries(server_node_object b0)

