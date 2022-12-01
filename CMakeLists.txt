cmake_minimum_required(VERSION 3.22)
project(ssl_tunnel C)

set(CMAKE_C_STANDARD 99)

enable_testing()

add_subdirectory(tests)
add_subdirectory(src)
