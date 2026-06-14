#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace glfw {
	class Window {
	public:
		Window(uint32_t width, uint32_t height, const char* title);
		~Window();

		[[nodiscard]] GLFWwindow*& window();
		[[nodiscard]] GLFWwindow* const& window() const;

		[[nodiscard]] GLFWwindow*& operator*();
		[[nodiscard]] GLFWwindow* const& operator*() const;

		[[nodiscard]] bool shouldClose() const;
		void setShouldClose(bool value);

		[[nodiscard]] vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);

	private:
		GLFWwindow* handle_;
	};
}
