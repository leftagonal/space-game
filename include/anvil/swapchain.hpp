#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace anvil {
	enum class SwapchainResult {
		Optimal = static_cast<int>(vk::Result::eSuccess),
		Suboptimal = static_cast<int>(vk::Result::eSuboptimalKHR),
		Recreate = static_cast<int>(vk::Result::eErrorOutOfDateKHR),
	};

	enum class Format {
		R8G8B8A8sRGB = static_cast<int>(vk::Format::eR8G8B8A8Srgb),
		B8G8R8A8sRGB = static_cast<int>(vk::Format::eB8G8R8A8Srgb),
	};

	enum class ColourSpace {
		sRGBNonlinear = static_cast<int>(vk::ColorSpaceKHR::eSrgbNonlinear),
	};

	struct WindowFormat {
		Format format;
		ColourSpace colourSpace;
	};

	enum class PresentMode {
		FIFO = static_cast<int>(vk::PresentModeKHR::eFifo),
		FIFORelaxed = static_cast<int>(vk::PresentModeKHR::eFifoRelaxed),
		Immediate = static_cast<int>(vk::PresentModeKHR::eImmediate),
		Mailbox = static_cast<int>(vk::PresentModeKHR::eMailbox),
	};

	class Window;
	class Device;
	class Semaphore;
	class Fence;

	struct SwapchainInfo {
		WindowFormat format;
		PresentMode presentMode;
		uint32_t imageCount;
	};

	struct SwapchainAcquisition {
		SwapchainResult result;
		uint32_t imageIndex;
	};

	class Swapchain {
	public:
		Swapchain(Device& device, Window& window, SwapchainInfo& info);
		Swapchain(Device& device, Window& window, SwapchainInfo& info, Swapchain& oldSwapchain);

		[[nodiscard]] vk::raii::SwapchainKHR& vkSwapchain();
		[[nodiscard]] const vk::raii::SwapchainKHR& vkSwapchain() const;

		[[nodiscard]] uint32_t imageCount() const;

		[[nodiscard]] SwapchainAcquisition acquire();
		[[nodiscard]] SwapchainAcquisition acquire(Semaphore& semaphore);
		[[nodiscard]] SwapchainAcquisition acquire(Fence& fence);
		[[nodiscard]] SwapchainAcquisition acquire(Semaphore& semaphore, Fence& fence);

	private:
		Device* device_;
		Window* window_;
		uint32_t imageCount_;

		vk::raii::SwapchainKHR vkSwapchain_;

		[[nodiscard]] vk::raii::SwapchainKHR makeSwapchain(const SwapchainInfo& info, Swapchain* oldSwapchain = nullptr);
	};
}
