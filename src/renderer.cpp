#include "renderer.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

float srgbToLinear(float c)
{
    if (c <= 0.04045f)
        return c / 12.92f;
    else
        return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

namespace fte
{

Renderer::Renderer(Window &window, Device &device) : window{window}, device{device}
{
    recreateSwapchain();
    createCommandBuffers();
}

Renderer::~Renderer()
{
    freeCommandBuffers();
}

void Renderer::recreateSwapchain()
{
    if (!window.isMinimized() && window.getSize().x > 0 && window.getSize().y > 0)
    {

        VkExtent2D extent = {static_cast<uint32_t>(window.getSize().x), static_cast<uint32_t>(window.getSize().y)};
        while (extent.width == 0 || extent.height == 0)
        {
            extent = {static_cast<uint32_t>(window.getSize().x), static_cast<uint32_t>(window.getSize().y)};
        }
        vkDeviceWaitIdle(device.getLogicalDevice());

        if (swapchain == nullptr)
        {
            swapchain = std::make_unique<Swapchain>(device, extent);
        }
        else
        {
            std::shared_ptr<Swapchain> oldSwapchain = std::move(swapchain);
            swapchain = std::make_unique<Swapchain>(device, extent, oldSwapchain);

            if (!oldSwapchain->compareSwapFormats(*swapchain.get()))
            {
                throw std::runtime_error("Swap chain image(or depth) format has changed!");
            }
        }
    }
}

void Renderer::createCommandBuffers()
{
    commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device.getLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Renderer::freeCommandBuffers()
{
    vkFreeCommandBuffers(device.getLogicalDevice(), device.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer Renderer::beginFrame()
{
    assert(!isFrameStarted && "Can't call beginFrame while already in progress");

    auto result = swapchain->acquireNextImage(&currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    return commandBuffer;
}

void Renderer::endFrame()
{
    assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
    auto commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    auto result = swapchain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.isResized())
    {
        window.resetResizedFlag();
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginSwapchainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Can't call beginSwapchainRenderPass if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchain->getRenderPass();
    renderPassInfo.framebuffer = swapchain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain->getSwapchainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {srgbToLinear(0.1f), srgbToLinear(0.2f), srgbToLinear(0.3), 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->getSwapchainExtent().width);
    viewport.height = static_cast<float>(swapchain->getSwapchainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapchain->getSwapchainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::endSwapchainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Can't call endSwapchainRenderPass if frame is not in progress");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't end render pass on command buffer from a different frame");
    vkCmdEndRenderPass(commandBuffer);
}

} // namespace fte
