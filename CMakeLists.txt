cmake_minimum_required(VERSION 3.30)
project(mvdb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler options
if(MSVC)
  add_compile_options(/W4 /WX-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wunused -O3 -fno-math-errno -fno-trapping-math -fno-signed-zeros -fassociative-math)
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


# Executables
find_package(fmt CONFIG REQUIRED)

add_executable(main tests/main.cpp)
target_link_libraries(main PRIVATE mvdb fmt::fmt)

add_executable(latency tests/latency.cpp)
target_link_libraries(latency PRIVATE mvdb fmt::fmt)
