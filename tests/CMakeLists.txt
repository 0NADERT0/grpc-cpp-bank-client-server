add_executable(banking_tests payment_test.cc)

target_link_libraries(
    banking_tests
    gtest
    gtest_main
    pthread
)

add_test(NAME BankingTests COMMAND banking_tests)