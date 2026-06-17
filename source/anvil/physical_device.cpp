#include <anvil/physical_device.hpp>

using namespace anvil;

PhysicalDevice::PhysicalDevice(const vk::raii::PhysicalDevice& physicalDevice)
	: vkPhysicalDevice_(physicalDevice) {
}

PhysicalDeviceInfo PhysicalDevice::info() const {
	auto properties = vkPhysicalDevice_.getProperties();

	return {
		.type = static_cast<PhysicalDeviceType>(properties.deviceType),
	};
}

vk::raii::PhysicalDevice& PhysicalDevice::vkPhysicalDevice() {
	return vkPhysicalDevice_;
}

const vk::raii::PhysicalDevice& PhysicalDevice::vkPhysicalDevice() const {
	return vkPhysicalDevice_;
}
