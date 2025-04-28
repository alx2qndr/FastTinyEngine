#include "window.hpp"

#include <iostream>
#include <stdexcept>

namespace fte
{
    Window::Window(int width, int height, const std::string &title)
    {
        create(width, height, title);
    }

    bool Window::create(int width, int height, const std::string &title)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            std::cerr << "Could not initialize SDL. SDL error: " + std::string(SDL_GetError()) << std::endl;
            return false;
        }

        if (!SDL_Vulkan_LoadLibrary(nullptr))
        {
            std::cerr << "Could not load Vulkan library. SDL error: " + std::string(SDL_GetError()) << std::endl;
            return false;
        }

        m_window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!m_window)
        {
            std::cerr << "Could not create SDL_Window. SDL error: " + std::string(SDL_GetError()) << std::endl;
            return false;
        }

        return true;
    }

    Window::~Window()
    {
        SDL_Vulkan_UnloadLibrary();
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void Window::pollEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                m_quitRequested = true;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                m_resized = true;
                break;

            case SDL_EVENT_WINDOW_MINIMIZED:
                m_minimized = true;
                break;

            case SDL_EVENT_WINDOW_RESTORED:
                m_minimized = false;
                break;

            default:
                break;
            }
        }
    }

    bool Window::createSurface(VkInstance instance, VkSurfaceKHR *pSurface, VkAllocationCallbacks *pAllocationCallbacks)
    {
        if (!SDL_Vulkan_CreateSurface(m_window, instance, pAllocationCallbacks, pSurface))
        {
            std::cerr << "Could not create surface. SDL error: " << std::string(SDL_GetError()) << std::endl;
            return false;
        }

        return true;
    }

    glm::vec2 Window::getSize() const
    {
        int width, height = 0;
        SDL_GetWindowSize(m_window, &width, &height);
        return glm::vec2(width, height);
    }
} // namespace fte
