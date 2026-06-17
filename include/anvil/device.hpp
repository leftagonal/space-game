#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <anvil/queue.hpp>

namespace anvil {
	class PhysicalDevice;
	class Window;

	struct DeviceQueueCounts {
		uint32_t graphics;
		uint32_t present;
		uint32_t transfer;
		uint32_t compute;
	};

	struct DeviceFeatures {
	};

	class Device {
	public:
		Device(PhysicalDevice& physicalDevice, Window& sampleWindow, const DeviceQueueCounts& queueCounts, const DeviceFeatures& features);

		[[nodiscard]] vk::raii::Device& vkDevice();
		[[nodiscard]] const vk::raii::Device& vkDevice() const;

		[[nodiscard]] PhysicalDevice& physicalDevice();
		[[nodiscard]] const PhysicalDevice& physicalDevice() const;

		[[nodiscard]] Queue getQueue(const QueueBinding& binding) const;

	private:
		struct QueueDefinition {
			uint32_t familyIndex;
			uint32_t queueIndex;
		};

		PhysicalDevice* physicalDevice_;
		DeviceFeatures features_;

		std::vector<QueueDefinition> graphicsDefinitions_;
		std::vector<QueueDefinition> presentDefinitions_;
		std::vector<QueueDefinition> transferDefinitions_;
		std::vector<QueueDefinition> computeDefinitions_;

		vk::raii::Device vkDevice_;

		[[nodiscard]] vk::raii::Device makeDevice(Window& sampleWindow, const DeviceQueueCounts& queueCounts);
	};
}
