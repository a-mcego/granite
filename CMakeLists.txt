cmake_minimum_required (VERSION 3.18)

cmake_policy (SET CMP0091 NEW)

project(granite VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(CMAKE)

option (DEBUG "Enable Debug Mode" OFF)
if (DEBUG)
    add_compile_definitions(DEBUG_MODE)
endif()

add_executable(granite
    src/main.cpp
    src/gl.cpp
    src/miniaudio.cpp
    src/opl2.cpp
)

find_package(glfw3 REQUIRED)

include_directories(include)

target_link_libraries(granite PRIVATE glfw)

if (WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
	target_compile_options(granite PRIVATE /EHsc /wd4805)
endif()


