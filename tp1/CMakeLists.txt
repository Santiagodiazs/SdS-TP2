cmake_minimum_required(VERSION 3.15)
project(tp1_benchmark_reference CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

get_filename_component(TP2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_executable(ss_tp1_benchmark
    main.cpp
    "${TP2_ROOT}/src/board_generation/Particle.cpp"
    "${TP2_ROOT}/src/board_generation/Tablero.cpp"
    "${TP2_ROOT}/src/utils/UniformDoubleGenerator.cpp"
    "${TP2_ROOT}/src/utils/UniformIntGenerator.cpp"
    "${TP2_ROOT}/src/cell_index_method/ParticleSystem.cpp"
)

target_include_directories(ss_tp1_benchmark PRIVATE
    "${TP2_ROOT}/include"
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources")
