add_library(alpha alpha)

add_executable(tweetwall main tweet)
target_link_libraries(tweetwall entities alpha json curl)

include_directories(3rdparty)

add_subdirectory(3rdparty)
add_subdirectory(iconv-alpha)
