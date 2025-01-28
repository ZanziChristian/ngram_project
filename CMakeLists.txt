cmake_minimum_required(VERSION 3.29)
project(sequential_bigram)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp -lomp -ftree-vectorize -march=native -mtune=native -fno-math-errno")

include_directories(/opt/homebrew/opt/libomp/include)
link_directories(/opt/homebrew/opt/libomp/lib)

add_executable(sequential_bigram_AoS bigrams_AoS.cpp)
add_executable(sequential_bigram_SoA bigrams_SoA.cpp)
add_executable(vectorized_bigram bigrams_vectorization.cpp)
add_executable(parallelized_bigram bigrams_parallel.cpp)
add_executable(parallelized_histogram histogram_parallel.cpp)

configure_file(mobydick.txt ${CMAKE_BINARY_DIR}/mobydick.txt COPYONLY)
