#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace glfw {
	enum class WindowHint {
		Resizable = GLFW_RESIZABLE,
		Decorated = GLFW_DECORATED,
	};

	class Context {
	public:
		Context();
		~Context();

		void pollEvents() const;
		void awaitEvents() const;
		void waitEvents(double timeout) const;

		void windowHint(WindowHint hint, bool value);

		[[nodiscard]] std::vector<const char*> requiredVulkanExtensions() const;
	};
}
