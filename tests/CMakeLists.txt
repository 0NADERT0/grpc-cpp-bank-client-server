# tests/CMakeLists.txt

enable_testing()
include(GoogleTest)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)
find_package(PostgreSQL REQUIRED)

add_executable(payment_service_tests
    payment_service_tests.cc
    ../src/db/postgres.cc
)

target_include_directories(payment_service_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${LIBPQXX_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(payment_service_tests
    PRIVATE
        gtest
        gtest_main
        gmock
        protolib
        gRPC::grpc++
        protobuf::libprotobuf
        ${LIBPQXX_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        pthread
)

gtest_discover_tests(payment_service_tests)
