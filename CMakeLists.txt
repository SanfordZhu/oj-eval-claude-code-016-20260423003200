cmake_minimum_required(VERSION 3.10)
project(bpt_database CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags for optimization and warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Create executable
add_executable(code main.cpp bpt.cpp)

# If we have header files
target_include_directories(code PRIVATE .)