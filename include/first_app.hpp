#pragma once

#include "descriptors.hpp"
#include "device.hpp"
#include "game_object.hpp"
#include "renderer.hpp"
#include "window.hpp"

#include <memory>
#include <vector>

namespace fte
{
class FirstApp
{
  public:
    FirstApp();
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

  private:
    Window window{320, 240, "Fast Little Game Engine"};
    Device device{window};
    Renderer renderer{window, device};

    std::unique_ptr<DescriptorPool> globalDescriptorPool{};
    GameObject::Map gameObjects;
};
} // namespace fte
