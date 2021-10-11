# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("MyProject")

# Add source to this project's executable.
add_executable (MyProject "MyProject.cpp" "MyProject.h")

# TODO: Add tests and install targets if needed.
