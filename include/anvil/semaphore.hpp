#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace anvil {
	class Device;

	class Semaphore {
	public:
		Semaphore(Device& device);

		[[nodiscard]] vk::raii::Semaphore& vkSemaphore();
		[[nodiscard]] const vk::raii::Semaphore& vkSemaphore() const;

	private:
		Device* device_;

		vk::raii::Semaphore vkSemaphore_;
	};
}
