cmake_minimum_required(VERSION 3.16)

project(amity_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(amity_solver
    src/main.cpp
    src/KnowledgeBase.cpp
    src/AmitySolver.cpp
)

target_include_directories(amity_solver PRIVATE src)
