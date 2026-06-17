#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace anvil {
	class Device;

	class Fence {
	public:
		Fence(Device& device, bool signal = false);

		[[nodiscard]] vk::raii::Fence& vkFence();
		[[nodiscard]] const vk::raii::Fence& vkFence() const;

	private:
		Device* device_;

		vk::raii::Fence vkFence_;
	};
}
