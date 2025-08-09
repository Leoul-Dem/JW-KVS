cmake_minimum_required(VERSION 3.20)
project(JW-KVS VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cxx" "src/*.cc")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS}
        internal/server.cpp
        internal/client.cpp)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)