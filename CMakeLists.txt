cmake_minimum_required(VERSION 3.30)
project(mvdb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MVDB_BUILD_PYTHON "Build the Python bindings" OFF)
option(MVDB_BUILD_MAIN "Build the main executable" ON)


if(MSVC)
  add_compile_options(/W4 /WX-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wunused)
endif()


# Core library 
add_library(mvdb STATIC
  src/index.cpp
  src/distance.cpp
)
set_target_properties(mvdb PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(mvdb PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)


# Executable
if(MVDB_BUILD_MAIN)
  find_package(fmt CONFIG REQUIRED)
  add_executable(main src/main.cpp)
  target_link_libraries(main PRIVATE mvdb fmt::fmt)
endif()


# Python bindings
if(MVDB_BUILD_PYTHON)
  set(PYBIND11_FINDPYTHON ON)
  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(_core src/pybindings.cpp)
  target_link_libraries(_core PRIVATE mvdb)
  set_target_properties(_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python/mvdb")
endif()
