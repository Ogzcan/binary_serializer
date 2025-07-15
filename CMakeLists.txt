cmake_minimum_required(VERSION 3.10)
project(crux_msg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(crux_msg STATIC
    include/binary_serializer/binary_serializer.hpp
    tests/unit_tests.cpp
)

target_include_directories(crux_msg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(crux_msg_tests tests/unit_tests.cpp)
target_link_libraries(crux_msg_tests PRIVATE crux_msg)

enable_testing()
add_test(NAME crux_msg_tests COMMAND crux_msg_tests)