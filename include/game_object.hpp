#pragma once

#include "model.hpp"
#include "texture.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace fte
{
struct TransformComponent
{
    glm::vec3 translation{};   
    glm::vec3 rotation{};      
    glm::vec3 scale{1.0f};     
    glm::vec3 rotationOrigin{};

    glm::mat4 mat4() const
    {
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 rotationMatrix = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    glm::mat3 normalMatrix() const
    {
        glm::mat4 modelMatrix = mat4();
        return glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    }
};

class GameObject
{
  public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, GameObject>;

    static GameObject createGameObject()
    {
        static id_t currentId = 0;
        return GameObject{currentId++};
    }

    GameObject(const GameObject &) = delete;
    GameObject &operator=(const GameObject &) = delete;
    GameObject(GameObject &&) = default;
    GameObject &operator=(GameObject &&) = default;

    id_t getId() const
    {
        return id;
    }

    glm::vec3 color{};
    TransformComponent transform = {};

    std::shared_ptr<Model> model;

  private:
    GameObject(id_t objId) : id{objId}
    {
    }

    id_t id;
};
} // namespace tre
