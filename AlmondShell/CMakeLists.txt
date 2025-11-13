cmake_minimum_required(VERSION 3.21)

project(AlmondShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ALMONDSHELL_SOURCES
    src/aengine.cpp
    src/acontext.cpp
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
        src/acontextmultiplexer_win.cpp
        src/aopenglplatform_win.cpp
        src/WinResource.rc
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND ALMONDSHELL_SOURCES
        src/acontextmultiplexer_linux.cpp
        src/aopenglplatform_linux.cpp
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

    # Raylib on Linux requires GLFW
    if(UNIX AND NOT APPLE)
        find_package(glfw3 CONFIG REQUIRED)
        target_link_libraries(almondshell PRIVATE glfw)
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
    # Always link against glad. Raylib no longer forwards the GL loader symbol on
    # Linux, so relying solely on its transitive export breaks the build. Linking
    # the official glad target keeps other platforms unchanged while restoring
    # `gladLoadGLLoader` for POSIX builds.
    target_link_libraries(almondshell PRIVATE glad::glad)
elseif(NOT TARGET glad::glad)
    message(FATAL_ERROR "glad was not found. Install it via vcpkg (./vcpkg install glad) "
        "or provide a glad::glad target before configuring AlmondShell.")
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
    find_package(X11 REQUIRED)
    if(X11_INCLUDE_DIR)
        target_include_directories(almondshell PRIVATE ${X11_INCLUDE_DIR})
    endif()
    target_link_libraries(almondshell PRIVATE dl m X11::X11 GL)
    if(TARGET X11::Xrandr)
        target_link_libraries(almondshell PRIVATE X11::Xrandr)
    elseif(X11_Xrandr_LIB)
        target_link_libraries(almondshell PRIVATE ${X11_Xrandr_LIB})
    endif()
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
