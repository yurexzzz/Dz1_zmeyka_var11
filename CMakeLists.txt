cmake_minimum_required(VERSION 3.16)
project(zmeyka LANGUAGES CXX)

add_executable(zmeyka main.cpp)
target_compile_features(zmeyka PRIVATE cxx_std_17)
set_target_properties(zmeyka PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(zmeyka PRIVATE -Wall -Wextra -Wpedantic)
