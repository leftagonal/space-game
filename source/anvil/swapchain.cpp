#include <anvil/device.hpp>
#include <anvil/fence.hpp>
#include <anvil/semaphore.hpp>
#include <anvil/swapchain.hpp>
#include <anvil/window.hpp>
#include <limits>

using namespace anvil;

Swapchain::Swapchain(Device& device, Window& window, SwapchainInfo& info)
	: device_(&device), window_(&window), vkSwapchain_(makeSwapchain(info)) {
	imageCount_ = vkSwapchain_.getImages().size();
}

Swapchain::Swapchain(Device& device, Window& window, SwapchainInfo& info, Swapchain& oldSwapchain)
	: device_(&device), window_(&window), vkSwapchain_(makeSwapchain(info, &oldSwapchain)) {
	imageCount_ = vkSwapchain_.getImages().size();
}

vk::raii::SwapchainKHR& Swapchain::vkSwapchain() {
	return vkSwapchain_;
}

const vk::raii::SwapchainKHR& Swapchain::vkSwapchain() const {
	return vkSwapchain_;
}

uint32_t Swapchain::imageCount() const {
	return imageCount_;
}

SwapchainAcquisition Swapchain::acquire() {
	auto timeout = std::numeric_limits<uint64_t>::max();
	auto badIndex = std::numeric_limits<uint32_t>::max();

	try {
		auto [acquireResult, imageIndex] = vkSwapchain_.acquireNextImage(timeout);

		return {
			.result = static_cast<SwapchainResult>(acquireResult),
			.imageIndex = imageIndex,
		};
	} catch (const vk::OutOfDateKHRError& error) {
		return {
			.result = SwapchainResult::Recreate,
			.imageIndex = badIndex,
		};
	}
}

SwapchainAcquisition Swapchain::acquire(Semaphore& semaphore) {
	auto timeout = std::numeric_limits<uint64_t>::max();
	auto badIndex = std::numeric_limits<uint32_t>::max();

	try {
		auto [acquireResult, imageIndex] = vkSwapchain_.acquireNextImage(timeout, semaphore.vkSemaphore());

		return {
			.result = static_cast<SwapchainResult>(acquireResult),
			.imageIndex = imageIndex,
		};
	} catch (const vk::OutOfDateKHRError& error) {
		return {
			.result = SwapchainResult::Recreate,
			.imageIndex = badIndex,
		};
	}
}

SwapchainAcquisition Swapchain::acquire(Fence& fence) {
	auto timeout = std::numeric_limits<uint64_t>::max();
	auto badIndex = std::numeric_limits<uint32_t>::max();

	try {
		auto [acquireResult, imageIndex] = vkSwapchain_.acquireNextImage(timeout, {}, fence.vkFence());

		return {
			.result = static_cast<SwapchainResult>(acquireResult),
			.imageIndex = imageIndex,
		};
	} catch (const vk::OutOfDateKHRError& error) {
		return {
			.result = SwapchainResult::Recreate,
			.imageIndex = badIndex,
		};
	}
}

SwapchainAcquisition Swapchain::acquire(Semaphore& semaphore, Fence& fence) {
	auto timeout = std::numeric_limits<uint64_t>::max();
	auto badIndex = std::numeric_limits<uint32_t>::max();

	try {
		auto [acquireResult, imageIndex] = vkSwapchain_.acquireNextImage(timeout, semaphore.vkSemaphore(), fence.vkFence());

		return {
			.result = static_cast<SwapchainResult>(acquireResult),
			.imageIndex = imageIndex,
		};
	} catch (const vk::OutOfDateKHRError& error) {
		return {
			.result = SwapchainResult::Recreate,
			.imageIndex = badIndex,
		};
	}
}

vk::raii::SwapchainKHR Swapchain::makeSwapchain(const SwapchainInfo& info, Swapchain* oldSwapchain) {
	auto& surface = window_->vkSurface();
	auto& physicalDevice = device_->physicalDevice().vkPhysicalDevice();
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

	vk::SwapchainCreateInfoKHR swapchainInfo = {
		.surface = surface,
		.minImageCount = std::clamp(info.imageCount, capabilities.minImageCount, capabilities.maxImageCount),
		.imageFormat = static_cast<vk::Format>(info.format.format),
		.imageColorSpace = static_cast<vk::ColorSpaceKHR>(info.format.colourSpace),
		.imageExtent = selectExtent(),
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.pQueueFamilyIndices = nullptr,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = static_cast<vk::PresentModeKHR>(info.presentMode),
		.clipped = false,
		.oldSwapchain = oldSwapchain ? *oldSwapchain->vkSwapchain() : nullptr,
	};

	return {device_->vkDevice(), swapchainInfo};
}

vk::Extent2D Swapchain::selectExtent() const {
    auto& surface = window_->vkSurface();
	auto& physicalDevice = device_->physicalDevice().vkPhysicalDevice();
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	auto sentinel = std::numeric_limits<uint32_t>::max();

	if (capabilities.currentExtent.width == sentinel || capabilities.currentExtent.height == sentinel) {
	    auto windowSize = window_->framebufferSize();

		return {
		    .width = std::clamp(windowSize.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		    .height = std::clamp(windowSize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
		};
	}

	return capabilities.currentExtent;
}
