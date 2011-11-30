add_library(ALPHA SHARED alpha.c alpha.h)

if(NOT GLIBC_SOURCE_FOLDER)
    message(FATAL_ERROR "Set CMake variable GLIBC_SOURCE_FOLDER to a glibc source folder (apt-get source eglibc)")
endif()

include_directories(
    ${GLIBC_SOURCE_FOLDER}/sysdeps/x86_64
    ${GLIBC_SOURCE_FOLDER}/iconvdata
    ${GLIBC_SOURCE_FOLDER}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(OUTPUT alpha.h
    COMMAND sh ./gen-8bit-gap.sh ${CMAKE_CURRENT_SOURCE_DIR}/ALPHA >
                                 ${CMAKE_CURRENT_BINARY_DIR}/alpha.h
    WORKING_DIRECTORY ${GLIBC_SOURCE_FOLDER}/iconvdata
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ALPHA
)
