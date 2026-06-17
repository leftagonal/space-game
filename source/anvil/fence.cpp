#include <anvil/device.hpp>
#include <anvil/fence.hpp>

using namespace anvil;

Fence::Fence(Device& device, bool signal)
	: device_(&device), vkFence_(device.vkDevice(), vk::FenceCreateInfo{.flags = signal ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags{}}) {
}

vk::raii::Fence& Fence::vkFence() {
	return vkFence_;
}

const vk::raii::Fence& Fence::vkFence() const {
	return vkFence_;
}
