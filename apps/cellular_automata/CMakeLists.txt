add_executable(cellular_automata_app
    main.cpp
)

target_link_libraries(cellular_automata_app
    PRIVATE
        cellular_automata_lib
        cell_index_method_lib
        board_generation_lib
        utils_lib
)