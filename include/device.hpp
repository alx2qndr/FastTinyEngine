#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL_vulkan.h>

#include "window.hpp"

namespace fte
{
struct SwapchainSupportInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilies
{
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool hasGraphicsFamily = false;
    bool hasPresentFamily = false;

    bool isValid() const
    {
        return hasGraphicsFamily && hasPresentFamily;
    }
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class Device
{
public:
    Device(Window& window);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    VkCommandPool getCommandPool() const { return commandPool; }
    VkDevice getLogicalDevice() const { return logicalDevice; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkSurfaceKHR getSurface() const { return surface; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }

    VkSampleCountFlagBits getMaxUsableSampleCount() const;

    SwapchainSupportInfo getSwapchainSupport() { return querySwapchainSupport(physicalDevice); }
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilies findQueueFamilies() { return findQueueFamilies(physicalDevice); }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

    void createImage(uint32_t width,
                     uint32_t height,
                     uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples,
                     VkFormat format,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkImage& image,
                     VkDeviceMemory& imageMemory,
                     const VkImageCreateInfo* customImageInfo = nullptr);

    VkPhysicalDeviceProperties properties;

private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    std::vector<const char*> getRequiredExtensions();
    void checkRequiredInstanceExtensions();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool checkValidationLayerSupport();
    SwapchainSupportInfo querySwapchainSupport(VkPhysicalDevice device);
    int ratePhysicalDeviceSuitability(VkPhysicalDevice device);
    bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
    bool checkPhysicalDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilies findQueueFamilies(VkPhysicalDevice device) const;
    std::string physicalDeviceTypeToString(VkPhysicalDeviceType type) const;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    Window& window;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDevice logicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
};
} // namespace lre
