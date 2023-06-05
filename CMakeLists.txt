cmake_minimum_required(VERSION 3.14)

project(ks)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 14)

add_subdirectory(test)

include_directories(include)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true include/*.h src/*.c)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)

