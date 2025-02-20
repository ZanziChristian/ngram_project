cmake_minimum_required(VERSION 3.29)
project(sequential_bigram)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xpreprocessor -fopenmp -lomp -ftree-vectorize -march=native -mtune=native -fno-math-errno")


include_directories(/opt/homebrew/opt/libomp/include)
link_directories(/opt/homebrew/opt/libomp/lib)

add_executable(s_b_count sequential_bigrams_count.c)
add_executable(p_b_count parallel_bigrams_count.c)

configure_file(mobydick.txt ${CMAKE_BINARY_DIR}/mobydick.txt COPYONLY)
