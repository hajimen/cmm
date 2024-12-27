cmake_minimum_required(VERSION 3.5)
project(cmm VERSION 0.1.0 LANGUAGES C CXX)

include_directories(Little-CMS/include)
include_directories(Little-CMS/src)
file(GLOB Little-CMS_src Little-CMS/src/*.c)
add_subdirectory(pybind11)
add_compile_definitions(VERSION_INFO="${VERSION_INFO}")
pybind11_add_module(cmm src/main.cpp ${Little-CMS_src})
