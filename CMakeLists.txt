cmake_minimum_required(VERSION 3.16)
project(FirstPass CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED)

add_executable(FirstPass src/main.cpp src/input/parser.cpp)

target_link_libraries(FirstPass PRIVATE Eigen3::Eigen)

target_include_directories(FirstPass PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/creator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver_op
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver_ac
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver_tran
    ${CMAKE_CURRENT_SOURCE_DIR}/src/output
)

target_compile_options(FirstPass PRIVATE -O3 -ffast-math -march=native)
