#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace anvil {
	enum class PhysicalDeviceType {
		Integrated = static_cast<int>(vk::PhysicalDeviceType::eIntegratedGpu),
		Software = static_cast<int>(vk::PhysicalDeviceType::eCpu),
		Discrete = static_cast<int>(vk::PhysicalDeviceType::eDiscreteGpu),
		Virtual = static_cast<int>(vk::PhysicalDeviceType::eVirtualGpu),
		Other = static_cast<int>(vk::PhysicalDeviceType::eOther),
	};

	struct PhysicalDeviceInfo {
		PhysicalDeviceType type;
	};

	class PhysicalDevice {
	public:
		PhysicalDevice(const vk::raii::PhysicalDevice& physicalDevice);

		[[nodiscard]] PhysicalDeviceInfo info() const;

		[[nodiscard]] vk::raii::PhysicalDevice& vkPhysicalDevice();
		[[nodiscard]] const vk::raii::PhysicalDevice& vkPhysicalDevice() const;

	private:
		vk::raii::PhysicalDevice vkPhysicalDevice_;
	};
}
