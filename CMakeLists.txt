cmake_minimum_required(VERSION 3.29)
project(sequential_bigram)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp -lomp -ftree-vectorize -march=native -mtune=native -fno-math-errno")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xpreprocessor -fopenmp -lomp -ftree-vectorize -march=native -mtune=native -fno-math-errno")


include_directories(/opt/homebrew/opt/libomp/include)
link_directories(/opt/homebrew/opt/libomp/lib)

add_executable(sequential_bigram_AoS bigrams_AoS.cpp)
add_executable(sequential_bigram_SoA bigrams_SoA.cpp)
add_executable(parallelized_bigram bigrams_parallel.cpp)
add_executable(parallelized_bigram_opt bigrams_parallel_opt.cpp)
add_executable(parallelized_histogram histogram_parallel.cpp)
add_executable(sequential_histogram sequential_histogram.c)
add_executable(parallel_histogram parallel_histogram.c)
add_executable(count wordcount.c)
add_executable(s_b_count sequential_bigrams_count.c)
add_executable(parallel_opt_histogram parallel_opt_histogram.c)
set(SOURCES
        main.cpp
        WordNgrams.cpp
        WordNgrams.h
)

# Creazione dell'eseguibile a partire dai file elencati
add_executable(main_cpp ${SOURCES})

target_include_directories(parallel_histogram PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(mobydick.txt ${CMAKE_BINARY_DIR}/mobydick.txt COPYONLY)
