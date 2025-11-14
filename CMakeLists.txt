cmake_minimum_required(VERSION 3.28)
project(Aegis VERSION 1.0.0 LANGUAGES CXX)

set(AEGIS_CXX_STANDARD 20 CACHE STRING "The C++ standard to build with")

set(CMAKE_CXX_STANDARD ${AEGIS_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AEGIS_BUILD_BACKEND_D3D121 "Enable the Direct3D 12 backend" ON)
option(AEGIS_BUILD_BACKEND_VULKAN "Enable the Vulkan backend" OFF)

option(BUILD_SHARED_LIBS "Build the 'Aegis' as a shared library" ON)

option(AEGIS_BUILD_TEST "Build the 'Aegis' unit tests" OFF)

find_program(AEGIS_DXC_EXECUTABLE
        NAMES dxc dxc.exe
        HINTS
          ENV "VULKAN_SDK"
          "C:/Program Files/DirectX Shader Compiler"
        DOC "Path to the DirectX Shader compiler (dxc) executable"
)

add_subdirectory(src)

if (AEGIS_BUILD_TEST)
    enable_testing()
    add_subdirectory(tests)
endif ()

# add_executable(aegis main.cpp)
