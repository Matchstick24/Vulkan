cmake_minimum_required(VERSION 3.10)
project(main VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(Vulkan REQUIRED)
find_package(glfw3 QUIET)

if (NOT glfw3_FOUND)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4   # pick a stable release tag
    )
    FetchContent_MakeAvailable(glfw)
endif()

file(GLOB SOURCES "src/*.cpp")
add_executable(main ${SOURCES})

# Adding Shaders into the build folder 

add_custom_target(Copy_Shaders ALL 
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Shaders
            ${CMAKE_BINARY_DIR}/Shaders
)

add_dependencies(main Copy_Shaders)

# linking and stuff 

target_link_libraries(main Vulkan::Vulkan glfw)
target_include_directories(main PRIVATE VulkanApp)