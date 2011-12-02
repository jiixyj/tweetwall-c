add_library(alpha alpha)

add_executable(tweetwall main tweet)
target_link_libraries(tweetwall entities alpha jansson curl)

add_subdirectory(iconv-alpha)
