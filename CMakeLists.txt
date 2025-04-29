cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(FastTinyEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_subdirectory("${CMAKE_SOURCE_DIR}/external")

set(SOURCES
    src/main.cpp   
    src/first_app.cpp
    src/systems/simple_render_system.cpp
    src/window.cpp
    src/swap_chain.cpp
    src/renderer.cpp
    src/pipeline.cpp
    src/model.cpp
    src/game_object.cpp
    src/descriptors.cpp
    src/camera.cpp
    src/buffer.cpp
    src/device.cpp
    src/texture.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC external::external)

if(WIN32)
    set(COPY_DEPENDENCIES SDL3::SDL3-shared)

    foreach(dep ${COPY_DEPENDENCIES})
        if(TARGET ${dep})
            list(APPEND COPY_TARGETS $<TARGET_FILE:${dep}>)
        endif()
    endforeach()

    if(COPY_TARGETS)
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                ${COPY_TARGETS}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            VERBATIM
        )
    endif()
endif()