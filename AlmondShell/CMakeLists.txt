cmake_minimum_required(VERSION 3.10...4.0)

cmake_policy(SET CMP0000 NEW)

project(updater LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(updater
    src/main.cpp
    src/updater_bootstrap.cpp
)
target_include_directories(updater PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(ALMOND_ENABLE_RAYLIB "Enable the Raylib backend" ON)

if(ALMOND_ENABLE_RAYLIB)
    target_compile_definitions(updater PRIVATE ALMOND_USING_RAYLIB)
    if(WIN32)
        target_link_libraries(updater PRIVATE opengl32)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(updater PRIVATE OpenGL::GL)
    endif()
endif()

if(MSVC)
    set(CMAKE_GENERATOR "Visual Studio 17 2022" CACHE STRING "Generator" FORCE)
endif()

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs/api
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating AlmondShell API documentation"
        VERBATIM)
else()
    message(STATUS "Doxygen not found; the 'docs' target will be skipped.")
endif()
