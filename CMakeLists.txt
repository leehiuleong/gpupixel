cmake_minimum_required(VERSION 3.10)
project(GPUPixelLookupFilter)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Include directories
include_directories(
    src
    src/core
    src/filter
    src/source
    src/third_party/stb
    src/third_party/glad/include
)

# Source files
set(SOURCES
    src/filter/lookup_filter.cc
    src/filter/filter.cc
    src/core/gpupixel_context.cc
    src/core/gl_program.cc
    src/core/framebuffer.cc
    src/core/source.cc
    src/core/target.cc
    src/util/util.cc
)

# Create library
add_library(gpupixel_lookup STATIC ${SOURCES})

# Link libraries
target_link_libraries(gpupixel_lookup
    OpenGL::GL
    glfw
    glad
)

# Example executable
add_executable(lookup_example examples/lookup_filter_example.cpp)
target_link_libraries(lookup_example gpupixel_lookup)

# Set compiler flags
if(MSVC)
    target_compile_options(gpupixel_lookup PRIVATE /W4)
else()
    target_compile_options(gpupixel_lookup PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(gpupixel_lookup PRIVATE GPUPIXEL_MAC)
elseif(WIN32)
    target_compile_definitions(gpupixel_lookup PRIVATE GPUPIXEL_WIN)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(gpupixel_lookup PRIVATE GPUPIXEL_LINUX)
endif()

# Installation
install(TARGETS gpupixel_lookup
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES src/filter/lookup_filter.h
    DESTINATION include/gpupixel/filter
)