#include "first_app.hpp"

#include "buffer.hpp"
#include "camera.hpp"
#include "systems/simple_render_system.hpp"
#include "texture.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace fte
{
FirstApp::FirstApp()
{
    globalDescriptorPool = DescriptorPool::Builder(device)
                               .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT)
                               .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swapchain::MAX_FRAMES_IN_FLIGHT)
                               .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swapchain::MAX_FRAMES_IN_FLIGHT)
                               .build();

    std::shared_ptr<Model> model = Model::createModelFromFile(device, "assets/models/tiny_frog/model.obj");

    auto object = GameObject::createGameObject();
    object.model = model;

    object.transform.translation = {0.0f, 0.0f, 0.0f};
    object.transform.scale = {3.0f, 3.0f, 3.0f};

    gameObjects.emplace(object.getId(), std::move(object));
}

FirstApp::~FirstApp()
{
}

void FirstApp::run()
{
    std::vector<std::unique_ptr<Buffer>> uboBuffers(Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++)
    {
        uboBuffers[i] = std::make_unique<Buffer>(device, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = DescriptorSetLayout::Builder(device)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                               .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                               .build();

    Texture texture(device, "../assets/models/tiny_frog/textures/baseColor.png");

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = texture.getSampler();
    imageInfo.imageLayout = texture.getImageLayout();
    imageInfo.imageView = texture.getImageView();

    std::vector<VkDescriptorSet> globalDescriptorSets(Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        DescriptorWriter(*globalSetLayout, *globalDescriptorPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo)
            .build(globalDescriptorSets[i]);
    }

    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapchainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};

    Camera camera;

    auto viewerObject = GameObject::createGameObject();
    viewerObject.transform.translation.z = 1.0f;

    auto currentTime = std::chrono::steady_clock::now();
    while (!window.isQuitRequested())
    {
        window.pollEvents();

        auto newTime = std::chrono::steady_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(75.0f), aspect, 0.1f, 4096.0f);

        for (auto &obj : gameObjects)
        {
            obj.second.transform.rotation.y += frameTime;
        }

        if (auto commandBuffer = renderer.beginFrame())
        {
            int frameIndex = renderer.getFrameIndex();
            FrameInfo frameInfo = {frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex],
                                   gameObjects};

            GlobalUbo ubo = {};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            renderer.beginSwapchainRenderPass(commandBuffer);

            simpleRenderSystem.renderGameObjects(frameInfo);

            renderer.endSwapchainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.getLogicalDevice());
}
} // namespace fte
