# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("MyProject")

# Add source to this project's executable.

add_executable (MyProject "MyProject.cpp" "MyProject.h" "App.h" "App.cpp" "RenderTarget.h" "Diligent.h" "Diligent.cpp" "RenderTarget.cpp" "DualRenderTarget.h" "DualRenderTarget.cpp" "Resource.h" "Resource.cpp" "submodules/stb/stb_image.cpp")

target_include_directories(MyProject PRIVATE ".")

# Copy assets to the binary directory

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/assets/)

# stb_image

target_include_directories( MyProject PRIVATE "submodules/stb" )

# DiligentEngine

target_include_directories( MyProject PRIVATE
        "submodules/DiligentCore/install/include" )

set(DILIGENT_LIB "submodules/DiligentCore/install/lib/Debug")

target_link_directories( MyProject PRIVATE ${DILIGENT_LIB} )

target_link_libraries( MyProject
        "DiligentCore"
        "glslangd"
        "HLSLd"
        "OGLCompilerd"
        "OSDependentd"
        "spirv-cross-cored"
        "glew-static"
        "GenericCodeGend"
        "MachineIndependentd"
        "GraphicsEngineOpenGL_64d"
        "GraphicsEngineD3D11_64d"
        "opengl32"
        "dxgi.lib"
        "d3d11.lib"
        "d3dcompiler.lib" )

# glm

add_subdirectory("submodules/glm")
target_include_directories( MyProject PRIVATE
        "submodules/glm" )
target_link_libraries( MyProject
        glm )

# glfw

add_subdirectory("submodules/glfw")
target_include_directories( MyProject PRIVATE
        "submodules/glfw/include" )
target_link_libraries( MyProject
        glfw )

# NewtooUI

add_subdirectory("submodules/NewtooUI")
target_include_directories( MyProject PRIVATE
        "submodules/NewtooUI/include" )
target_link_libraries( MyProject
        NewtooUI )

# TODO: Add tests and install targets if needed.
