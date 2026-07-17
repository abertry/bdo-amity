cmake_minimum_required(VERSION 3.16)

project(amity_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(amity_core
    src/KnowledgeBase.cpp
    src/AmitySolver.cpp
    src/CommandLine.cpp
)
target_include_directories(amity_core PUBLIC src)
target_compile_definitions(amity_core PRIVATE
    KNOWLEDGE_DATABASE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/knowledge.json"
)

add_executable(amity_solver src/main.cpp)
target_link_libraries(amity_solver PRIVATE amity_core)

include(CTest)
enable_testing()
if(BUILD_TESTING)
    add_executable(amity_tests test/AmitySolverTest.cpp)
    target_link_libraries(amity_tests PRIVATE amity_core)
    add_test(NAME amity_tests COMMAND amity_tests)
endif()
