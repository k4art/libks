add_executable(test_ipv4 ipv4.test.c)
add_test(NAME test_ipv4 COMMAND test_ipv4)
target_link_libraries(test_ipv4 PUBLIC ks)

add_executable(test_loop_run_empty loop_run_empty.test.c)
add_test(NAME test_loop_run_empty COMMAND test_loop_run_empty)
target_link_libraries(test_loop_run_empty PUBLIC ks)
