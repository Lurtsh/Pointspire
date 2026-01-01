cmake_minimum_required(VERSION 3.26)
project(Pointspire VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
find_package(Git REQUIRED)

set(TGA_PRIVATE_URL "git@gitlab.lrz.de:tuini15/gpu-pro/public/TGA.git")
set(TGA_PUBLIC_URL "git@github.com:Estard/TGA.git")

message(STATUS "Checking access to private TGA repository...")
execute_process(
        COMMAND ${CMAKE_COMMAND} -E env GIT_TERMINAL_PROMPT=0 ${GIT_EXECUTABLE} ls-remote --exit-code ${TGA_PRIVATE_URL} HEAD
        RESULT_VARIABLE TGA_PRIVATE_ACCESS
        OUTPUT_QUIET
        ERROR_QUIET
)

if(TGA_PRIVATE_ACCESS EQUAL 0)
    message(STATUS "SUCCESS: Using University version of TGA.")
    set(TGA_REPO ${TGA_PRIVATE_URL})
    set(TGA_TAG "master")
else()
    message(STATUS "WARNING: Access denied. Defaulting to the public version of TGA.")
    set(TGA_REPO ${TGA_PUBLIC_URL})
    set(TGA_TAG "master")
endif()

FetchContent_Declare(
        tga
        GIT_REPOSITORY ${TGA_REPO}
        GIT_TAG ${TGA_TAG}
        UPDATE_DISCONNECTED 1
)

FetchContent_MakeAvailable(tga)

if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Og")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -03 -march=native")
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(include)

set(HEADERS Application.hpp)

set(SOURCES Application.cpp)

list(TRANSFORM HEADERS PREPEND "include/")
list(TRANSFORM SOURCES PREPEND "src/")

add_executable(Pointspire main.cpp ${HEADERS} ${SOURCES})

target_include_directories(Pointspire PRIVATE include)

add_subdirectory(shaders)
add_dependencies(Pointspire shaders)

target_link_libraries(Pointspire PRIVATE tga_vulkan tga_utils ${CMAKE_THREAD_LIBS_INIT})