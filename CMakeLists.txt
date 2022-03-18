cmake_minimum_required(VERSION 3.21)
project(VoxelDeformations)
#set(CMAKE_UNITY_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC
    src/main.cpp
 "src/utils.h")

add_executable(VoxelDeformations
    ${SRC}
)

add_subdirectory(focus)

target_include_directories(VoxelDeformations
        PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/glm
        ${CMAKE_SOURCE_DIR}/libs/SDL2
)

target_link_libraries(
        VoxelDeformations
        focus
        Setupapi.lib
        ${CMAKE_SOURCE_DIR}/libs/SDL2-static.lib
        ${CMAKE_SOURCE_DIR}/libs/SDL2main.lib
        msvcrtd.lib
        winmm.lib
        imm32.lib
        version.lib
        d3d11.lib
        d3d12.lib
        libcmt.lib
        dxgi.lib
        d3dcompiler.lib
        dxguid.lib
        winmm.lib
        gdi32
        opengl32
)
