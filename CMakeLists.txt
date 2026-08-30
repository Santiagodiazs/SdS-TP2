cmake_minimum_required(VERSION 3.16)
project(SimulacionDeSistemas CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# --- utils ---
add_library(utils_lib
    src/utils/UniformDoubleGenerator.cpp
    src/utils/UniformIntGenerator.cpp
)

# --- board_generation ---
add_library(board_generation_lib
    src/board_generation/Particle.cpp
    src/board_generation/Tablero.cpp
)

# --- cell_index_method ---
add_library(cell_index_method_lib
    src/cell_index_method/ParticleSystem.cpp
)
target_link_libraries(cell_index_method_lib PRIVATE board_generation_lib)

# --- cellular_automata (TP2) ---
add_library(cellular_automata_lib
    src/cellular_automata/CellularAutomataSystem.cpp
    src/cellular_automata/VicsekRule.cpp
    src/cellular_automata/VoterRule.cpp
)
target_link_libraries(cellular_automata_lib
    PRIVATE
        board_generation_lib
        cell_index_method_lib
        utils_lib
)

add_subdirectory(apps/cellular_automata)