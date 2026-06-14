#include <glfw/context.hpp>

using namespace glfw;

Context::Context() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

Context::~Context() {
	glfwTerminate();
}

void Context::pollEvents() const {
	glfwPollEvents();
}

void Context::awaitEvents() const {
	glfwWaitEvents();
}

void Context::waitEvents(double timeout) const {
	glfwWaitEventsTimeout(timeout);
}

void Context::windowHint(WindowHint hint, bool value) {
	glfwWindowHint(static_cast<int>(hint), value);
}

[[nodiscard]] std::vector<const char*> Context::requiredVulkanExtensions() const {
	uint32_t count = 0;
	auto names = glfwGetRequiredInstanceExtensions(&count);

	return {names, names + count};
}
