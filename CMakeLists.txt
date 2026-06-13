cmake_minimum_required(VERSION 3.16...3.29)
project(cmm LANGUAGES C CXX)

include_directories(Little-CMS/include)
include_directories(Little-CMS/src)
add_compile_definitions(VERSION_INFO="${VERSION_INFO}")

set(LCMS2_BUILD_SHARED OFF)
set(LCMS2_BUILD_STATIC ON)
set(LCMS2_BUILD_TOOLS OFF)
set(LCMS2_BUILD_TESTS OFF)
set(LCMS2_WITH_THREADS OFF)
add_subdirectory(Little-CMS)
set_property(TARGET lcms2 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_subdirectory(pybind11)
pybind11_add_module(cmm src/main.cpp)
target_link_libraries(cmm PRIVATE lcms2)
