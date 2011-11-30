add_executable(tweetwall main)
target_link_libraries(tweetwall entities jansson curl)

add_subdirectory(iconv-alpha)
