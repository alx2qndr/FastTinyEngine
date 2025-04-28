#pragma once

#include "camera.hpp"
#include "game_object.hpp"

#include <vulkan/vulkan.h>

namespace fte {
struct PointLight {
    glm::vec4 position {};
    glm::vec4 color {};
};

struct GlobalUbo {
    glm::mat4 projection { 1.f };
    glm::mat4 view { 1.f };
    glm::mat4 inverseView { 1.f };
};

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    Camera& camera;
    VkDescriptorSet globalDescriptorSet;
    GameObject::Map& gameObjects;
};
} // namespace tre
