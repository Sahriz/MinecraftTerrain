cmake_minimum_required(VERSION 3.15)
project(MinecraftClone CXX)

# Set C++ 20 standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If your vendor libraries have their own CMakeLists.txt, you can add them here.
# Otherwise, we will just include their source directly in your targets.

add_subdirectory(Vendor)
add_subdirectory(Core)
add_subdirectory(Program)

set_property(TARGET Program PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")