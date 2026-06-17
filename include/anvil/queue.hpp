#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace anvil {
	enum class QueueUsage {
		Graphics,
		Present,
		Transfer,
		Compute,
	};

	struct QueueBinding {
		QueueUsage usage;
		uint32_t index;
	};

	class Queue {
	public:
		Queue(const vk::raii::Queue& queue);

		[[nodiscard]] vk::raii::Queue& vkQueue();
		[[nodiscard]] const vk::raii::Queue& vkQueue() const;

	private:
		vk::raii::Queue vkQueue_;
	};
}
