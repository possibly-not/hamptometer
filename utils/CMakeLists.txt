aux_source_directory(. DIR_UTIL_SRCS)

add_library(Utils ${DIR_UTIL_SRCS})
target_link_libraries(Utils PUBLIC
    pico_stdlib
)