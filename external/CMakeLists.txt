include(FetchContent)

add_library(external INTERFACE)
add_library(external::external ALIAS external)

find_package(SDL3 QUIET CONFIG)
if (NOT SDL3_FOUND)
    message(WARNING "SDL3 package not found. Attempting to fetch it from the repository...")
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.10
    )
    FetchContent_MakeAvailable(SDL3)
endif()

find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan package not found. Please install Vulkan SDK and try again.")
endif()

find_package(glm QUIET CONFIG)
if (NOT glm_FOUND)
    message(WARNING "glm package not found. Attempting to fetch it from the repository...")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/stb_image")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tiny_obj_loader")

target_link_libraries(external INTERFACE
    SDL3::SDL3
    Vulkan::Vulkan
    glm::glm
    stb_image
    tiny_obj_loader
)
