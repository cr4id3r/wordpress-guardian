cmake_minimum_required(VERSION 3.26)
project(wordpress_guardian)

set(CMAKE_CXX_STANDARD 17)
find_package(CURL REQUIRED)

add_executable(wordpress_guardian main.cpp)

target_link_libraries(wordpress_guardian PRIVATE CURL::libcurl)
