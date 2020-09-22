cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(test_parsing)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include(get_cpm.cmake)

CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY fmtlib/fmt
    GIT_TAG 7.0.1
    )
CPMAddPackage(
    NAME magic_enum
    GITHUB_REPOSITORY Neargye/magic_enum
    GIT_TAG v0.6.6
    )
CPMAddPackage(
    NAME ranges-v3
    GITHUB_REPOSITORY ericniebler/range-v3
    GIT_TAG 0.11.0
    )
CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.8.0
    )
CPMAddPackage(
    NAME ctre
    GITHUB_REPOSITORY hanickadot/compile-time-regular-expressions
    VERSION 2.9.1
    )
CPMAddPackage(
    NAME nanobench
    GITHUB_REPOSITORY martinus/nanobench
    VERSION 4.2.0
    )
CPMAddPackage(
    NAME catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 2.13.1
    )
CPMAddPackage(
    NAME pegtl
    GITHUB_REPOSITORY taocpp/PEGTL
    GIT_TAG 2.8.3
    )

list(APPEND C_OPTS
    -Wall
    -Wextra
    -Wshadow
    -Wunused-variable
    -flto
    )

include(gps_parser.cmake)

add_library(gps_parser_std STATIC
    ${CMAKE_CURRENT_LIST_DIR}/gps_parser.hpp
    ${CMAKE_CURRENT_LIST_DIR}/gps_parser_std.cpp
    )
target_link_libraries(gps_parser_std PRIVATE magic_enum)
target_include_directories(gps_parser_std PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(gps_parser_peg STATIC
    ${CMAKE_CURRENT_LIST_DIR}/gps_parser.hpp
    ${CMAKE_CURRENT_LIST_DIR}/gps_parser_peg.cpp
    )
target_link_libraries(gps_parser_peg PRIVATE magic_enum taocpp::pegtl)
target_include_directories(gps_parser_peg PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(gps_bench main.cpp)
target_link_libraries(gps_bench PUBLIC
    gps_parser
    nanobench
    spdlog
    fmt
    magic_enum
    -flto
    )
target_compile_options(gps_bench PUBLIC ${C_OPTS})

add_executable(gps_bench_std main.cpp)
target_link_libraries(gps_bench_std PUBLIC
    gps_parser
    nanobench
    spdlog
    fmt
    magic_enum
    -flto
    )
target_compile_options(gps_bench_std PUBLIC ${C_OPTS})

add_executable(test_parsing main_test.cpp test_parsing.cpp)
target_link_libraries(test_parsing PUBLIC Catch2::Catch2 gps_parser)

add_executable(test_parsing_std main_test.cpp test_parsing.cpp)
target_link_libraries(test_parsing_std PUBLIC Catch2::Catch2 gps_parser_std)

add_executable(test_parsing_peg main_test.cpp test_parsing.cpp)
target_link_libraries(test_parsing_peg PUBLIC Catch2::Catch2 gps_parser_peg)
