cmake_minimum_required(VERSION 3.14)

project(ks)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

enable_testing()

add_subdirectory(test)
add_subdirectory(examples)

include_directories(include)
include_directories(src)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true include/*.h src/*.c)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} uring pthread) 
