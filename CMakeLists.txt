cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
cmake_policy(VERSION 2.8)
set(NAME Pyrrhasterized)
project(${NAME})

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

if (UNIX AND NOT APPLE)
	find_package(glm REQUIRED)
	find_package(glfw3 REQUIRED)
elseif (WIN32)
	add_library(glfw3 STATIC IMPORTED)
	set_target_properties(glfw3 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/lib/glfw3.lib)
elseif (APPLE)
	message(FATAL_ERROR "Apple not supported")
endif()

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS ${Vulkan_LIBRARY})
endif()


add_subdirectory(third_party)
add_subdirectory(shaders)
add_subdirectory(src)

