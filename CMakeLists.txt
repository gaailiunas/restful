cmake_minimum_required(VERSION 3.10)
project(restfulapi)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(restfulapi src/main.c src/event_loop.c src/client.c src/write_queue.c)
target_include_directories(restfulapi PRIVATE include)
target_compile_options(restfulapi PRIVATE -Wall -Wextra)
