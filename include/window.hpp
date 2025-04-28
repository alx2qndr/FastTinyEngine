#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <string>

namespace fte
{
    class Window
    {
    public:
        Window() = default;

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        Window(Window &&) = delete;
        Window &operator=(Window &&) = delete;

        Window(int width, int height, const std::string &title);
        bool create(int width, int height, const std::string &title);

        ~Window();

        void pollEvents();

        bool createSurface(VkInstance instance, VkSurfaceKHR *pSurface, VkAllocationCallbacks *pAllocationCallbacks);

        void resetResizedFlag() { m_resized = false; }

        glm::vec2 getSize() const;

        bool isQuitRequested() const { return m_quitRequested; }

        bool isResized() const { return m_resized; }
        bool isMinimized() const { return m_minimized; }

    private:
        SDL_Window *m_window = nullptr;

        bool m_quitRequested = false;
        bool m_resized = false;
        bool m_minimized = false;
    };
} // namespace fte
