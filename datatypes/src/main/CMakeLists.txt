cmake_minimum_required(VERSION 3.10)
project(MyArrowProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Arrow and link it
find_package(Arrow REQUIRED)

# Assuming your code is in a file named main.cpp
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE arrow_shared)
