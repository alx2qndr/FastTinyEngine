#pragma once

#include "device.hpp"

#include <string>
#include <memory>

#include <vulkan/vulkan.h>

namespace fte {
	class Texture {
	public:
		Texture(Device& device, const std::string& filePath);
		~Texture();

		Texture(const Texture& other) = delete;
		Texture& operator=(Texture& other) = delete;

		Texture(Texture&& other) = delete;
		Texture& operator=(Texture&& other) = delete;

		VkSampler getSampler() const { return sampler; }
		VkImageView getImageView() const { return imageView; }
		VkImageLayout getImageLayout() const { return imageLayout; }

		static std::shared_ptr<Texture> createTextureFromFile(Device& device, const std::string& filePath);

	private:
		void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		void generateMipmaps();

		int width, height, mipLevels;

		Device& device;

		VkImage image;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkFormat imageFormat;
		VkImageLayout imageLayout;
		VkSampler sampler;
	};
}