#include <anvil/queue.hpp>

using namespace anvil;

Queue::Queue(const vk::raii::Queue& queue)
	: vkQueue_(queue) {
}

vk::raii::Queue& Queue::vkQueue() {
	return vkQueue_;
}

const vk::raii::Queue& Queue::vkQueue() const {
	return vkQueue_;
}
