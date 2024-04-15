aux_source_directory(. DIR_net_INFO)
add_library(Info ${DIR_net_INFO})


target_include_directories(Info PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/..
)
target_link_libraries(Info PUBLIC
        pico_stdlib
)
