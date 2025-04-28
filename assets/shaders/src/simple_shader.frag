#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D image;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 normalMatrix;
} push;

void main() {
  vec3 imageColor = texture(image, fragUV).rgb;
  outColor = vec4(imageColor, 1.0);
}
