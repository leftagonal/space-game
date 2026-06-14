#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cmath>
#include <vector>

#include <context.hpp>
#include <window.hpp>

int main() {
	anvil::ContextFeatures contextFeatures = {
		.debugging = true,
	};

	anvil::ApplicationInfo applicationInfo = {
		.name = "Space Game",
		.version = {0, 1, 0},
	};

	anvil::Context context{applicationInfo, contextFeatures};

	anvil::WindowInfo windowInfo = {
		.title = "Space Game",
		.extent = {800, 600},
	};

	anvil::WindowFeatures windowFeatures = {
		.resizable = false,
		.decorated = true,
	};

	anvil::Window window{context, windowInfo, windowFeatures};

	auto physicalDevices = context.vkInstance().enumeratePhysicalDevices();
	auto physicalDevice = physicalDevices.front();

	auto deviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

	std::vector<const char*> requiredDeviceExtensions = {
		vk::KHRSwapchainExtensionName,
	};

	for (auto& requirement : requiredDeviceExtensions) {
		auto condition = [&](const auto& candidate) {
			return candidate.extensionName == std::string_view(requirement);
		};

		auto candidate = std::find_if(deviceExtensions.begin(), deviceExtensions.end(), condition);

		if (candidate == deviceExtensions.end()) {
			throw std::runtime_error("required extension is missing!");
		}
	}

	std::vector<float> priorities = {1.0};

	vk::DeviceQueueCreateInfo queueInfo = {
		.queueFamilyIndex = 0,
		.queueCount = 1,
		.pQueuePriorities = priorities.data(),
	};

	vk::PhysicalDeviceVulkan13Features vulkan13Features = {
		.synchronization2 = true,
		.dynamicRendering = true,
	};

	vk::DeviceCreateInfo deviceInfo = {
		.pNext = &vulkan13Features,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueInfo,
		.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
		.ppEnabledExtensionNames = requiredDeviceExtensions.data(),
	};

	vk::raii::Device device{physicalDevice, deviceInfo};
	vk::raii::Queue queue = device.getQueue(0, 0);

	auto& surface = window.vkSurface();
	auto preTransform = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentTransform;

	vk::SwapchainCreateInfoKHR swapchainInfo = {
		.surface = surface,
		.minImageCount = 3,
		.imageFormat = vk::Format::eR8G8B8A8Unorm,
		.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
		.imageExtent = {800, 600},
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.pQueueFamilyIndices = nullptr,
		.preTransform = preTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = vk::PresentModeKHR::eFifo,
		.clipped = false,
		.oldSwapchain = nullptr,
	};

	vk::raii::SwapchainKHR swapchain{device, swapchainInfo};

	auto images = swapchain.getImages();

	vk::SemaphoreCreateInfo semaphoreInfo = {};
	vk::FenceCreateInfo fenceInfo = {
		.flags = vk::FenceCreateFlagBits::eSignaled,
	};

	std::vector<vk::raii::Semaphore> awaitAcquire;
	std::vector<vk::raii::Semaphore> signalRender;
	std::vector<vk::raii::Fence> inFlight;

	for (size_t i = 0; i < images.size(); ++i) {
		awaitAcquire.emplace_back(device, semaphoreInfo);
		signalRender.emplace_back(device, semaphoreInfo);
		inFlight.emplace_back(device, fenceInfo);
	}

	vk::CommandPoolCreateInfo poolInfo = {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = 0,
	};

	vk::raii::CommandPool commandPool{device, poolInfo};

	vk::CommandBufferAllocateInfo cmdAllocInfo = {
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = static_cast<uint32_t>(images.size()),
	};

	auto commandBuffers = device.allocateCommandBuffers(cmdAllocInfo);
	auto& glfwContext = context.glfwContext();

	size_t frame = 0;
	float time = 0.0;

	while (!window.glfwWindow().shouldClose()) {
		glfwContext.pollEvents();

		auto& fence = inFlight[frame];
		auto& cmd = commandBuffers[frame];

		auto waitResult = device.waitForFences({fence}, true, UINT64_MAX);
		device.resetFences({fence});

		auto [acquireResult, imageIndex] = swapchain.acquireNextImage(UINT64_MAX, awaitAcquire[frame]);

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo{});

		vk::ImageMemoryBarrier2 toClear = {
			.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
			.srcAccessMask = vk::AccessFlags2{},
			.dstStageMask = vk::PipelineStageFlagBits2::eClear,
			.dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
			.oldLayout = vk::ImageLayout::eUndefined,
			.newLayout = vk::ImageLayout::eTransferDstOptimal,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = images[imageIndex],
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
		};

		vk::DependencyInfo toClearDep = {
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &toClear,
		};

		cmd.pipelineBarrier2(toClearDep);

		vk::ClearColorValue clearColor{std::array<float, 4>{
			-std::sin(time),
			std::cos(time),
			std::sin(time),
			1.0f,
		}};

		cmd.clearColorImage(images[imageIndex], vk::ImageLayout::eTransferDstOptimal, clearColor, {vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

		vk::ImageMemoryBarrier2 toPresent = {
			.srcStageMask = vk::PipelineStageFlagBits2::eClear,
			.srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
			.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
			.dstAccessMask = vk::AccessFlags2{},
			.oldLayout = vk::ImageLayout::eTransferDstOptimal,
			.newLayout = vk::ImageLayout::ePresentSrcKHR,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = images[imageIndex],
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
		};

		vk::DependencyInfo toPresentDep = {
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &toPresent,
		};

		cmd.pipelineBarrier2(toPresentDep);

		cmd.end();

		vk::SemaphoreSubmitInfo waitInfo = {
			.semaphore = *awaitAcquire[frame],
			.stageMask = vk::PipelineStageFlagBits2::eClear,
		};

		vk::SemaphoreSubmitInfo signalInfo = {
			.semaphore = *signalRender[frame],
			.stageMask = vk::PipelineStageFlagBits2::eClear,
		};

		vk::CommandBufferSubmitInfo cmdInfo = {
			.commandBuffer = *cmd,
		};

		vk::SubmitInfo2 submitInfo = {
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitInfo,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalInfo,
		};

		queue.submit2({submitInfo}, fence);

		vk::PresentInfoKHR presentInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &*signalRender[frame],
			.swapchainCount = 1,
			.pSwapchains = &*swapchain,
			.pImageIndices = &imageIndex,
		};

		auto presentResult = queue.presentKHR(presentInfo);

		frame = (frame + 1) % images.size();
		time += 0.01;
	}

	device.waitIdle();
}
