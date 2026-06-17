#include <anvil/window.hpp>

using namespace anvil;

Window::Window(Context& context, const WindowInfo& info, const WindowFeatures& features)
	: info_(info), features_(features), context_(&context), glfwWindow_(makeWindow()), vkSurface_(makeSurface()) {
}

[[nodiscard]] glfw::Window& Window::glfwWindow() {
	return glfwWindow_;
}

[[nodiscard]] const glfw::Window& Window::glfwWindow() const {
	return glfwWindow_;
}

[[nodiscard]] vk::raii::SurfaceKHR& Window::vkSurface() {
	return vkSurface_;
}

[[nodiscard]] const vk::raii::SurfaceKHR& Window::vkSurface() const {
	return vkSurface_;
}

[[nodiscard]] Extent2D Window::framebufferSize() const {
    Extent2D extent;

    glfwWindow_.getFramebufferSize(extent.width, extent.height);

    return extent;
}

[[nodiscard]] glfw::Window Window::makeWindow() {
	auto& glfwContext = context_->glfwContext();

	glfwContext.windowHint(glfw::WindowHint::Resizable, features_.resizable);
	glfwContext.windowHint(glfw::WindowHint::Decorated, features_.decorated);

	return {info_.extent.width, info_.extent.height, info_.title.data()};
}

[[nodiscard]] vk::raii::SurfaceKHR Window::makeSurface() {
	return glfwWindow_.createSurface(context_->vkInstance());
}
