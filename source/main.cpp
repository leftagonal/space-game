#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cmath>
#include <vector>

#include <anvil/context.hpp>
#include <anvil/device.hpp>
#include <anvil/fence.hpp>
#include <anvil/semaphore.hpp>
#include <anvil/swapchain.hpp>
#include <anvil/window.hpp>

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
		.resizable = true,
		.decorated = true,
	};

	anvil::Window window{context, windowInfo, windowFeatures};

	auto physicalDevices = context.physicalDevices();
	auto physicalDevice = physicalDevices.back();

	anvil::DeviceQueueCounts deviceQueueCounts = {
		.graphics = 1,
		.present = 1,
		.transfer = 1,
		.compute = 0,
	};

	anvil::DeviceFeatures deviceFeatures = {};

	anvil::Device device(physicalDevice, window, deviceQueueCounts, deviceFeatures);

	anvil::Queue graphicsQueue = device.getQueue({anvil::QueueUsage::Graphics, 0});
	anvil::Queue presentQueue = device.getQueue({anvil::QueueUsage::Present, 0});
	anvil::Queue transferQueue = device.getQueue({anvil::QueueUsage::Transfer, 0});

	anvil::SwapchainInfo swapchainInfo = {
		.format = {
			.format = anvil::Format::B8G8R8A8sRGB,
			.colourSpace = anvil::ColourSpace::sRGBNonlinear,
		},
		.presentMode = anvil::PresentMode::FIFO,
		.imageCount = 3,
	};

	anvil::Swapchain swapchain{device, window, swapchainInfo};

	std::vector<anvil::Semaphore> awaitAcquire;
	std::vector<anvil::Semaphore> signalRender;
	std::vector<anvil::Fence> inFlight;

	for (size_t i = 0; i < swapchain.imageCount(); ++i) {
		awaitAcquire.emplace_back(device);
		signalRender.emplace_back(device);
		inFlight.emplace_back(device, true);
	}

	vk::CommandPoolCreateInfo poolInfo = {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = 0,
	};

	vk::raii::CommandPool commandPool{device.vkDevice(), poolInfo};

	vk::CommandBufferAllocateInfo cmdAllocInfo = {
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = static_cast<uint32_t>(swapchain.imageCount()),
	};

	auto commandBuffers = device.vkDevice().allocateCommandBuffers(cmdAllocInfo);
	auto& glfwContext = context.glfwContext();

	size_t frame = 0;
	float time = 0.0;

	while (!window.glfwWindow().shouldClose()) {
		glfwContext.pollEvents();

		auto& fence = inFlight[frame];
		auto& cmd = commandBuffers[frame];

		auto waitResult = device.vkDevice().waitForFences({fence.vkFence()}, true, UINT64_MAX);
		device.vkDevice().resetFences({fence.vkFence()});

	reacquire:
		auto [acquireResult, imageIndex] = swapchain.acquire(awaitAcquire[frame]);

		if (acquireResult == anvil::SwapchainResult::Recreate) {
			anvil::Swapchain newSwapchain{device, window, swapchainInfo, swapchain};
			swapchain = std::move(newSwapchain);

			goto reacquire;
		}

		auto images = swapchain.vkSwapchain().getImages();

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
			.semaphore = *awaitAcquire[frame].vkSemaphore(),
			.stageMask = vk::PipelineStageFlagBits2::eClear,
		};

		vk::SemaphoreSubmitInfo signalInfo = {
			.semaphore = *signalRender[frame].vkSemaphore(),
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

		graphicsQueue.vkQueue().submit2({submitInfo}, fence.vkFence());

		vk::PresentInfoKHR presentInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &*signalRender[frame].vkSemaphore(),
			.swapchainCount = 1,
			.pSwapchains = &*swapchain.vkSwapchain(),
			.pImageIndices = &imageIndex,
		};

		try {
			auto presentResult = presentQueue.vkQueue().presentKHR(presentInfo);
		} catch (const vk::OutOfDateKHRError& error) {
			anvil::Swapchain newSwapchain{device, window, swapchainInfo, swapchain};
			swapchain = std::move(newSwapchain);
		}

		frame = (frame + 1) % images.size();
		time += 0.01;
	}

	device.vkDevice().waitIdle();
}
