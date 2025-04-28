cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(FastTinyEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_subdirectory("${CMAKE_SOURCE_DIR}/external")

add_executable(${PROJECT_NAME} 
    "${CMAKE_SOURCE_DIR}/src/main.cpp"   
    "${CMAKE_SOURCE_DIR}/src/first_app.cpp"
    "${CMAKE_SOURCE_DIR}/src/systems/simple_render_system.cpp"
    "${CMAKE_SOURCE_DIR}/src/window.cpp"
    "${CMAKE_SOURCE_DIR}/src/swap_chain.cpp"
    "${CMAKE_SOURCE_DIR}/src/renderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/pipeline.cpp"
    "${CMAKE_SOURCE_DIR}/src/model.cpp"
    "${CMAKE_SOURCE_DIR}/src/game_object.cpp"
    "${CMAKE_SOURCE_DIR}/src/descriptors.cpp"
    "${CMAKE_SOURCE_DIR}/src/camera.cpp"
    "${CMAKE_SOURCE_DIR}/src/buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/device.cpp"
    "${CMAKE_SOURCE_DIR}/src/device.cpp"
    "${CMAKE_SOURCE_DIR}/src/texture.cpp"

)

target_link_libraries(${PROJECT_NAME} PUBLIC external::external)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include")
