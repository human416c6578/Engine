cmake_minimum_required(VERSION 3.20)
project(Engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/third_party
)


# Find Vulkan
find_package(Vulkan REQUIRED)


# Find GLFW
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})


# Find Assimp
find_package(assimp REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})


# imgui + imgui_filedialog sources
file(GLOB IMGUI_SOURCES
    ${PROJECT_SOURCE_DIR}/include/third_party/imgui/*.cpp
    ${PROJECT_SOURCE_DIR}/include/third_party/imgui_filedialog/*.cpp
)


# Engine sources
file(GLOB_RECURSE ENGINE_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)


# Executable
add_executable(EngineApp
    ${ENGINE_SOURCES}
    ${IMGUI_SOURCES}
)


# Link libraries
target_link_libraries(EngineApp
    PRIVATE
    ${Vulkan_LIBRARY}
    ${GLFW_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    dl
    pthread
    X11
)


# Compiler warnings
if(MSVC)
    target_compile_options(EngineApp PRIVATE /W4 /WX)
else()
    target_compile_options(EngineApp PRIVATE -Wall -Wextra -Wpedantic)
endif()
