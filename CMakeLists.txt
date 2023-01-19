cmake_minimum_required(VERSION 2.8)

project(my_compiler)

include_directories(
    ${PROJECT_SOURCE_DIR}
)

add_library(
    MY_COMPILER_SRC
    ${PROJECT_SOURCE_DIR}/lexer.cpp
)

add_executable(my_compiler ${PROJECT_SOURCE_DIR}/main.cpp)
target_link_libraries(my_compiler MY_COMPILER_SRC)