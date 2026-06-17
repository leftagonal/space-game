#pragma once

#include <cstdint>
#include <string_view>

#include <anvil/context.hpp>
#include <glfw/window.hpp>

namespace anvil {
	struct Extent2D {
		using Type = uint32_t;

		Type width;
		Type height;
	};

	struct WindowInfo {
		std::string_view title;
		Extent2D extent;
	};

	struct WindowFeatures {
		bool resizable;
		bool decorated;
	};

	class Window {
	public:
		Window(Context& context, const WindowInfo& info, const WindowFeatures& features);

		[[nodiscard]] glfw::Window& glfwWindow();
		[[nodiscard]] const glfw::Window& glfwWindow() const;

		[[nodiscard]] vk::raii::SurfaceKHR& vkSurface();
		[[nodiscard]] const vk::raii::SurfaceKHR& vkSurface() const;

		[[nodiscard]] Extent2D framebufferSize() const;

	private:
		WindowInfo info_;
		WindowFeatures features_;
		Context* context_;

		glfw::Window glfwWindow_;
		vk::raii::SurfaceKHR vkSurface_;

		[[nodiscard]] glfw::Window makeWindow();
		[[nodiscard]] vk::raii::SurfaceKHR makeSurface();
	};
}
