cmake_minimum_required(VERSION 2.8)
project(pom)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")

aux_source_directory(. SRC)

add_executable(pom-vertical ${SRC})
find_package(PNG REQUIRED)
target_link_libraries(pom-vertical ${PNG_LIBRARY})