######################################################
cmake_minimum_required(VERSION 3.20)
project(BankingService)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(other/googletest)

enable_testing()
add_subdirectory(tests)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(Protobuf REQUIRED)
unset(CMAKE_FIND_PACKAGE_PREFER_CONFIG)

find_package(gRPC CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

include_directories(${LIBPQXX_INCLUDE_DIRS})
link_directories(${LIBPQXX_LIBRARY_DIRS})

find_package(PostgreSQL REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIRS} ${PQXX_INCLUDE_DIRS})

add_library(dblib
    src/db/postgres.cc
    src/db/postgres.h
    src/db/databaseInterface.h
)
target_include_directories(dblib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBPQXX_INCLUDE_DIRS}
)

add_library(protolib proto/payment_service.proto)
target_link_libraries(protolib gRPC::grpc++)
target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET protolib LANGUAGE cpp)
protobuf_generate(TARGET protolib LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")


add_executable(server src/server.cc)
target_include_directories(server PRIVATE ${LIBPQXX_INCLUDE_DIRS})
target_link_libraries(
    server
    PRIVATE
    protolib
    dblib
    gRPC::grpc++
    protobuf::libprotobuf
    ${LIBPQXX_LIBRARIES}
    ${LIBPQ_LIBRARIES}
)


add_executable(client src/client.cc)
target_link_libraries(client protolib)