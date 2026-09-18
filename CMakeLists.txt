cmake_minimum_required(VERSION 3.30)
project(mvdb
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()


find_package(pybind11 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)


# Core library
add_library(mini_vector_db STATIC
  src/index.cpp
  src/distance.cpp
)

set_target_properties(mini_vector_db PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(mini_vector_db
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)


# Python bindings
pybind11_add_module(_core
  src/pybindings.cpp
)

set_target_properties(_core PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python/mini_vector_db"
)

target_link_libraries(_core
  PRIVATE
  mini_vector_db
)

# Executables
add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE mini_vector_db fmt::fmt)

add_executable(recall_benchmark src/recall.cpp)
target_link_libraries(recall_benchmark PRIVATE mini_vector_db fmt::fmt)

install(TARGETS _core LIBRARY DESTINATION mini_vector_db)
