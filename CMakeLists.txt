cmake_minimum_required(VERSION 3.20)
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


# WebAssembly bindings (configure with emcmake).
option(MVDB_BUILD_WASM "Build JavaScript/WebAssembly bindings" ${EMSCRIPTEN})
if(EMSCRIPTEN)
  set(MVDB_EXAMPLES_DEFAULT OFF)
else()
  set(MVDB_EXAMPLES_DEFAULT ON)
endif()
option(MVDB_BUILD_EXAMPLES "Build native demo executables (requires fmt)" ${MVDB_EXAMPLES_DEFAULT})

if(MVDB_BUILD_WASM)
  if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "MVDB_BUILD_WASM requires Emscripten. Configure with emcmake cmake.")
  endif()

  # Enable catching core exceptions so the bindings can turn them into JS errors.
  target_compile_options(mvdb PRIVATE -fexceptions)
  add_executable(mvdb_wasm src/wasmbindings.cpp)
  target_link_libraries(mvdb_wasm PRIVATE mvdb)
  target_compile_options(mvdb_wasm PRIVATE -fexceptions)
  target_link_options(mvdb_wasm PRIVATE
    --bind
    -fexceptions
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sENVIRONMENT=web,node
    -sALLOW_MEMORY_GROWTH=1
    -sFILESYSTEM=0
    -sEXPORT_NAME="createMVDB"
  )
  set_target_properties(mvdb_wasm PROPERTIES
    OUTPUT_NAME mvdb-core
    SUFFIX ".mjs"
  )
endif()

# Native executables
if(MVDB_BUILD_EXAMPLES)
  find_package(fmt CONFIG REQUIRED)

  add_executable(main tests/cpp/main.cpp)
  target_link_libraries(main PRIVATE mvdb fmt::fmt)

  add_executable(latency tests/cpp/latency.cpp)
  target_link_libraries(latency PRIVATE mvdb fmt::fmt)
endif()

