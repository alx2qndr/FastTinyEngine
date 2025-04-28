#include "swap_chain.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace fte
{

Swapchain::Swapchain(Device& deviceRef, VkExtent2D extent) : device {deviceRef}, windowExtent {extent}
{
    createSwapchain();
    init();
}

Swapchain::Swapchain(Device& deviceRef, VkExtent2D extent, std::shared_ptr<Swapchain> previous)
    : device {deviceRef}, windowExtent {extent}, oldSwapchain {previous}
{
    createSwapchain();
    init();
}

void Swapchain::init()
{
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        if (swapchainImages[i] == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Swap chain image is null!");
        }

        swapchainImageViews[i]
            = createImageView(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    createRenderPass();
    createDepthResources();
    createColorResources();
    createFramebuffers();
    createSyncObjects();
}

Swapchain::~Swapchain()
{
    for (auto framebuffer : swapchainFramebuffers)
    {
        vkDestroyFramebuffer(device.getLogicalDevice(), framebuffer, nullptr);
    }

    for (auto imageView : swapchainImageViews)
    {
        vkDestroyImageView(device.getLogicalDevice(), imageView, nullptr);
    }

    swapchainImageViews.clear();

    if (swapchain != nullptr)
    {
        vkDestroySwapchainKHR(device.getLogicalDevice(), swapchain, nullptr);
        swapchain = nullptr;
    }

    for (int i = 0; i < depthImages.size(); i++)
    {
        vkDestroyImageView(device.getLogicalDevice(), depthImageViews[i], nullptr);
        vkDestroyImage(device.getLogicalDevice(), depthImages[i], nullptr);
        vkFreeMemory(device.getLogicalDevice(), depthImageMemorys[i], nullptr);
    }

    for (int i = 0; i < colorImages.size(); i++)
    {
        vkDestroyImageView(device.getLogicalDevice(), colorImageViews[i], nullptr);
        vkDestroyImage(device.getLogicalDevice(), colorImages[i], nullptr);
        vkFreeMemory(device.getLogicalDevice(), colorImageMemorys[i], nullptr);
    }

    vkDestroyRenderPass(device.getLogicalDevice(), renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device.getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device.getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.getLogicalDevice(), inFlightFences[i], nullptr);
    }
}

VkResult Swapchain::acquireNextImage(uint32_t* imageIndex)
{
    vkWaitForFences(
        device.getLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(device.getLogicalDevice(),
                                            swapchain,
                                            std::numeric_limits<uint64_t>::max(),
                                            imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE,
                                            imageIndex);

    return result;
}

VkResult Swapchain::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
{
    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device.getLogicalDevice(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.getLogicalDevice(), 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;

    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void Swapchain::createSwapchain()
{
    SwapchainSupportInfo swapchainSupport = device.getSwapchainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    swapchainImageFormat = surfaceFormat.format;

    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    swapchainExtent = chooseSwapExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device.getSurface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilies indices = device.findQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->swapchain;

    if (vkCreateSwapchainKHR(device.getLogicalDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = swapchainExtent;
}

VkImageView
Swapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device.getLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void Swapchain::createRenderPass()
{
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = device.getMaxUsableSampleCount();
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = device.getMaxUsableSampleCount();
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve {};
    colorAttachmentResolve.format = swapchainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.getLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void Swapchain::createFramebuffers()
{
    swapchainFramebuffers.resize(imageCount());
    for (size_t i = 0; i < imageCount(); i++)
    {
        if (!colorImageViews[i] || !depthImageViews[i] || !swapchainImageViews[i])
        {
            throw std::runtime_error("Framebuffer attachments not initialized!");
        }

        std::array<VkImageView, 3> attachments = {colorImageViews[i], depthImageViews[i], swapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.getLogicalDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers[i])
            != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Swapchain::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();
    swapchainDepthFormat = depthFormat;
    VkExtent2D swapchainExtent = getSwapchainExtent();

    depthImages.resize(imageCount());
    depthImageMemorys.resize(imageCount());
    depthImageViews.resize(imageCount());

    for (int i = 0; i < depthImages.size(); i++)
    {
        device.createImage(swapchainExtent.width,
                           swapchainExtent.height,
                           1,
                           device.getMaxUsableSampleCount(),
                           depthFormat,
                           VK_IMAGE_TILING_OPTIMAL,
                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           depthImages[i],
                           depthImageMemorys[i]);

        depthImageViews[i] = createImageView(depthImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
}

void Swapchain::createColorResources()
{
    VkFormat colorFormat = swapchainImageFormat;

    colorImages.resize(imageCount());
    colorImageMemorys.resize(imageCount());
    colorImageViews.resize(imageCount());

    for (int i = 0; i < colorImages.size(); i++)
    {
        device.createImage(swapchainExtent.width,
                           swapchainExtent.height,
                           1,
                           device.getMaxUsableSampleCount(),
                           colorFormat,
                           VK_IMAGE_TILING_OPTIMAL,
                           VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           colorImages[i],
                           colorImageMemorys[i]);
        colorImageViews[i] = createImageView(colorImages[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void Swapchain::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i])
                != VK_SUCCESS
            || vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])
                != VK_SUCCESS
            || vkCreateFence(device.getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = windowExtent;
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VkFormat Swapchain::findDepthFormat()
{
    return device.findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                      VK_IMAGE_TILING_OPTIMAL,
                                      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace tre
