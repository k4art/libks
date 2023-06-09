find_package(Python REQUIRED)

set(TEST_CASES
    singlethreaded/tcp_single_hello
    singlethreaded/tcp_single_echo
    singlethreaded/tcp_blackhole
    singlethreaded/tcp_ping_pong_server

    multithreaded/async_sort
)

foreach(TEST_CASE ${TEST_CASES})
  set(TEST_CASE_EXE    ${TEST_CASE})
  set(TEST_CASE_SRC    ${TEST_CASE}.c)
  set(TEST_CASE_PY     ${TEST_CASE}.test.py)
  set(TEST_CASE_PY_ABS ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_CASE_PY})

  string(REPLACE "/" "-" TEST_CASE_NAME ${TEST_CASE_EXE})

  add_executable(${TEST_CASE_NAME} ${TEST_CASE_SRC})
  target_link_libraries(${TEST_CASE_NAME} PUBLIC ks)

  add_test(NAME    ${TEST_CASE_NAME}
           COMMAND ${Python_EXECUTABLE} ${TEST_CASE_PY_ABS}
                   --executable $<TARGET_FILE:${TEST_CASE_NAME}>)
endforeach(TEST_CASE)
