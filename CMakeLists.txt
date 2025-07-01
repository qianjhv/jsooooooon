cmake_minimum_required(VERSION 3.16)

project(jsooooooon)

add_library(jsooooooon jsooooooon.c)
add_executable(json_test test.c)
target_link_libraries(json_test jsooooooon)
