find_package(LibXml2 REQUIRED)
include_directories(SYSTEM ${LIBXML2_INCLUDE_DIR})
add_library(entities entities)
target_link_libraries(entities ${LIBXML2_LIBRARIES})
