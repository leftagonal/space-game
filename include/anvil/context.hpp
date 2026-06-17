#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <string_view>

#include <anvil/physical_device.hpp>
#include <glfw/context.hpp>

namespace anvil {
	struct Version {
		using Type = uint32_t;

		Type major;
		Type minor;
		Type patch;
	};

	struct ContextFeatures {
		bool debugging;
	};

	struct ApplicationInfo {
		std::string_view name;
		Version version;
	};

	class Context {
	public:
		Context(const ApplicationInfo& applicationInfo, const ContextFeatures& features);

		[[nodiscard]] glfw::Context& glfwContext();
		[[nodiscard]] const glfw::Context& glfwContext() const;

		[[nodiscard]] vk::raii::Context& vkContext();
		[[nodiscard]] const vk::raii::Context& vkContext() const;

		[[nodiscard]] vk::raii::Instance& vkInstance();
		[[nodiscard]] const vk::raii::Instance& vkInstance() const;

		[[nodiscard]] std::vector<PhysicalDevice> physicalDevices() const;

	private:
		ApplicationInfo applicationInfo_;
		ContextFeatures features_;

		glfw::Context glfwContext_;
		vk::raii::Context vkContext_;
		vk::raii::Instance vkInstance_;

		[[nodiscard]] vk::raii::Instance makeInstance();
	};
}
