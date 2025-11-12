cmake_minimum_required(VERSION 3.21)

project(AlmondShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ALMONDSHELL_SOURCES
    src/aengine.cpp
    src/acontext.cpp
    src/acontextmultiplexer.cpp
    src/aeditor.cpp
    src/agui.cpp
    src/afontrenderer.cpp
    src/afilewatch.cpp
    src/ascriptingsystem.cpp
    src/acompiler.cpp
    src/updater_bootstrap.cpp
)

if(WIN32)
    list(APPEND ALMONDSHELL_SOURCES
        src/WinResource.rc
    )
endif()

add_executable(almondshell ${ALMONDSHELL_SOURCES})

target_include_directories(almondshell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stb
)

option(ALMOND_ENABLE_RAYLIB "Enable the Raylib backend" ON)

if(ALMOND_ENABLE_RAYLIB)
    target_compile_definitions(almondshell PRIVATE ALMOND_USING_RAYLIB)
    find_package(raylib CONFIG REQUIRED)
    target_link_libraries(almondshell PRIVATE raylib)
endif()

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(almondshell PRIVATE SDL3::SDL3)

find_package(SDL3_image CONFIG QUIET)
if(SDL3_image_FOUND)
    target_link_libraries(almondshell PRIVATE SDL3_image::SDL3_image)
endif()

find_package(OpenGL COMPONENTS OpenGL QUIET)
if(OpenGL_FOUND)
    target_link_libraries(almondshell PRIVATE OpenGL::GL)
elseif(WIN32)
    target_link_libraries(almondshell PRIVATE opengl32)
endif()

find_package(glad CONFIG QUIET)
if(glad_FOUND)
    target_link_libraries(almondshell PRIVATE glad::glad)
endif()

find_package(glm CONFIG QUIET)
if(glm_FOUND)
    target_link_libraries(almondshell PRIVATE glm::glm)
endif()

find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
    target_link_libraries(almondshell PRIVATE fmt::fmt)
endif()

find_package(asio CONFIG QUIET)
if(asio_FOUND)
    target_link_libraries(almondshell PRIVATE asio::asio)
endif()

find_package(Threads REQUIRED)
target_link_libraries(almondshell PRIVATE Threads::Threads)

if(UNIX AND NOT APPLE)
    target_link_libraries(almondshell PRIVATE dl m)
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
