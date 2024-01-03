enable_testing()

add_executable(
  hello_test
  hello_test.cc
)
add_executable(
    log_test
    logtest.cc
)
target_link_libraries(
  hello_test
  GTest::gtest_main
)
target_link_libraries(
    log_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hello_test)
gtest_discover_tests(log_test)