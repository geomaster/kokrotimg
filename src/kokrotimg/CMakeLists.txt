add_library(kokrotimg SHARED
    common.c
    instrumentation-posix.c
    macrocosm.c
    microcosm.c
    kokrotimg.c
)

file(MAKE_DIRECTORY ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/kokrotimg)

add_custom_command(TARGET kokrotimg POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_SOURCE_DIR}/kokrotimg.h ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/kokrotimg)

add_custom_command(TARGET kokrotimg POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_SOURCE_DIR}/debug.h ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/kokrotimg)


set(kokrotimg_INCLUDE_DIRS ${CMAKE_INCLUDE_OUTPUT_DIRECTORY} PARENT_SCOPE)

install(TARGETS kokrotimg
    DESTINATION "lib")

install(DIRECTORY include
        DESTINATION "include/kokrotimg"
)
