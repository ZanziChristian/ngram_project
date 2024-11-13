cmake_minimum_required(VERSION 3.29)
project(sequential_bigram)

set(CMAKE_CXX_STANDARD 20)

add_executable(sequential_bigram main.cpp)

configure_file(mobydick.txt ${CMAKE_BINARY_DIR}/mobydick.txt COPYONLY)
