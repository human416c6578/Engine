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
    src/renderer/se_postprocess.cpp
)

# GLSL → SPIR-V compilation
set(SHADER_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/shaders)

file(GLOB SHADER_SOURCES
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
    ${SHADER_DIR}/*.comp
)

find_program(GLSLC glslc HINTS
    $ENV{VULKAN_SDK}/Bin
    $ENV{VULKAN_SDK}/bin
)

if(NOT GLSLC)
  message(FATAL_ERROR "glslc not found — install Vulkan SDK")
endif()

foreach(SHADER ${SHADER_SOURCES})
  get_filename_component(FILE_NAME ${SHADER} NAME_WE)
  set(SPIRV ${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv)

  add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${FILE_NAME}"
    )

  list(APPEND SPIRV_BINARIES ${SPIRV})
endforeach()

# Create custom target so shaders always build
add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARIES})

# Ensure Engine builds shaders first
add_dependencies(EngineApp Shaders)

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
