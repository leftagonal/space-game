#include <glfw/window.hpp>

using namespace glfw;

Window::Window(uint32_t width, uint32_t height, const char* title) {
	int iwidth = static_cast<int>(width);
	int iheight = static_cast<int>(height);

	handle_ = glfwCreateWindow(iwidth, iheight, title, nullptr, nullptr);
}

Window::~Window() {
	glfwDestroyWindow(handle_);
}

GLFWwindow*& Window::window() {
	return handle_;
}

GLFWwindow* const& Window::window() const {
	return handle_;
}

GLFWwindow*& Window::operator*() {
	return handle_;
}

GLFWwindow* const& Window::operator*() const {
	return handle_;
}

bool Window::shouldClose() const {
	return glfwWindowShouldClose(handle_);
}

void Window::setShouldClose(bool value) {
	glfwSetWindowShouldClose(handle_, value);
}

vk::raii::SurfaceKHR Window::createSurface(vk::raii::Instance& instance) {
	VkSurfaceKHR surface = nullptr;

	auto result = glfwCreateWindowSurface(*instance, handle_, nullptr, &surface);

	return {instance, surface};
}
