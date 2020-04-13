cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(unix_fd_pass C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(DEFAULT_WARN_FLAGS -Wall -Wextra -Wpedantic)

add_executable(client client.c)
add_executable(server server.c)
target_compile_options(client PRIVATE ${DEFAULT_WARN_FLAGS})
target_compile_options(server PRIVATE ${DEFAULT_WARN_FLAGS})
