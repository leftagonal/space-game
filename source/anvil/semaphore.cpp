#include <anvil/device.hpp>
#include <anvil/semaphore.hpp>

using namespace anvil;

Semaphore::Semaphore(Device& device)
	: device_(&device), vkSemaphore_(device.vkDevice(), vk::SemaphoreCreateInfo{}) {
}

vk::raii::Semaphore& Semaphore::vkSemaphore() {
	return vkSemaphore_;
}

const vk::raii::Semaphore& Semaphore::vkSemaphore() const {
	return vkSemaphore_;
}
