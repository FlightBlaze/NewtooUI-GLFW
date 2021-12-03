# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("MyProject")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source to this project's executable.

if( APPLE )
        set( PLATFORM_SOURCE "platform/MacOS/LayerManager.h" "platform/MacOS/LayerManager.m" )
endif()

add_executable (MyProject "MyProject.cpp" "MyProject.h" "App.h" "App.cpp" "RenderTarget.h" "Diligent.h" "Diligent.cpp" "RenderTarget.cpp" "Resource.h" "Resource.cpp" "submodules/stb/stb_image.cpp" "TextRenderer.h" "TextRenderer.cpp" "Shape.h" "Shape.cpp" "MathExtras.h" "MathExtras.cpp" "Elements.h" "Elements.cpp" "Context.h" "Context.cpp" "Gizmos.h" "Gizmos.cpp" ${PLATFORM_SOURCE})

target_include_directories(MyProject PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Copy assets to the binary directory

if( APPLE )
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/assets/)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/assets/)
else()
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/assets/)
endif()

# stb_image

target_include_directories( MyProject PRIVATE "submodules/stb" )

# QuartzCore

if( APPLE )
        enable_language(OBJCXX)
        find_library( QUARTZ_CORE QuartzCore REQUIRED )
        find_library( METAL Metal REQUIRED )
        target_link_libraries( MyProject ${QUARTZ_CORE} ${METAL} )
endif()

# SDL

target_include_directories( MyProject PRIVATE
        "submodules/SDL/include" )
add_subdirectory( "submodules/SDL" )
target_link_libraries( MyProject SDL2-static )

# DiligentEngine

target_include_directories( MyProject PRIVATE
        "submodules/DiligentCore/install/include" )

add_subdirectory( "submodules/DiligentCore" )

target_link_libraries( MyProject Diligent-GraphicsEngineVk-shared Diligent-Common )



# set(DILIGENT_LIB "submodules/DiligentCore/install/lib/Debug")

# target_link_directories( MyProject PRIVATE ${DILIGENT_LIB} )

# if(APPLE)
#         target_link_libraries( MyProject
#                 "DiligentCore"
#                 "HLSL"
#                 "OSDependent"
#                 "spirv-cross-core"
#                 "SPIRV-Tools-opt"
#                 "SPIRV-Tools"
#                 "SPIRV"
#                 "GenericCodeGen"
#                 "MachineIndependent"
#                 "GraphicsEngineVk" )
# else()
#         target_link_libraries( MyProject
#                 "DiligentCore"
#                 "glslangd"
#                 "HLSLd"
#                 "OGLCompilerd"
#                 "OSDependentd"
#                 "spirv-cross-cored"
#                 "glew-static"
#                 "GenericCodeGend"
#                 "MachineIndependentd"
#                 "GraphicsEngineOpenGL_64d"
#                 "GraphicsEngineD3D11_64d"
#                 "opengl32"
#                 "dxgi.lib"
#                 "d3d11.lib"
#                 "d3dcompiler.lib" )

# endif()

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

# tube

add_subdirectory("submodules/tube")
target_include_directories( MyProject PRIVATE
        "submodules/tube/include" )
target_link_libraries( MyProject
        tube )

# NewtooUI

add_subdirectory("submodules/NewtooUI")
target_include_directories( MyProject PRIVATE
        "submodules/NewtooUI/include" )
target_link_libraries( MyProject
        NewtooUI )

# TODO: Add tests and install targets if needed.

# blazevg

add_subdirectory("submodules/blazevg")
target_include_directories( MyProject PRIVATE
        "submodules/blazevg/include" )
target_link_libraries( MyProject
        blazevg )
