cmake_minimum_required(VERSION 3.1)
project(ZMQ1TO1)

# 设置C++标准为 C++ 11
set(CMAKE_CXX_STANDARD 11)

# 设置C++标准为 C++ 14
# set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g")

add_executable(zmq1to1 test.cpp)
target_link_libraries(zmq1to1 pthread)




