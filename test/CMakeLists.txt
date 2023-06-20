set(TEST_CASES
    ipv4
    loop_run_empty
    wq_fifo
    wq_pushes_batches_to_global
    loop_run_works-single
    loop_run_works-multi
)

foreach(TEST_CASE ${TEST_CASES})
  set(TEST_CASE_EXE    test_${TEST_CASE})
  set(TEST_CASE_SRC    ${TEST_CASE}.test.c)

  add_executable(${TEST_CASE_EXE} ${TEST_CASE_SRC})
  target_link_libraries(${TEST_CASE_EXE} PUBLIC ks)

  add_test(NAME    ${TEST_CASE}
           COMMAND ${TEST_CASE_EXE})
endforeach(TEST_CASE)
