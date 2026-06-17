#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <chrono>
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
		.presentMode = anvil::PresentMode::Immediate,
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

	auto lastTime = std::chrono::steady_clock::now();
	int frameCount = 0;
	auto images = swapchain.vkSwapchain().getImages();
	std::vector<vk::raii::ImageView> imageViews;
	for (auto& image : images) {
		vk::ImageViewCreateInfo viewInfo = {
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = vk::Format::eB8G8R8A8Srgb,
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
		};

		imageViews.emplace_back(device.vkDevice(), viewInfo);
	}

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

			images = swapchain.vkSwapchain().getImages();
			imageViews.clear();

			for (auto& image : images) {
				vk::ImageViewCreateInfo viewInfo = {
					.image = image,
					.viewType = vk::ImageViewType::e2D,
					.format = vk::Format::eB8G8R8A8Srgb,
					.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
				};

				imageViews.emplace_back(device.vkDevice(), viewInfo);
			}

			goto reacquire;
		}

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo{});

		vk::ImageMemoryBarrier2 toRender = {
			.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
			.srcAccessMask = vk::AccessFlags2{},
			.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
			.oldLayout = vk::ImageLayout::eUndefined,
			.newLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = images[imageIndex],
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
		};

		cmd.pipelineBarrier2(vk::DependencyInfo{
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &toRender,
		});

		vk::RenderingAttachmentInfo colorAttachment = {
			.imageView = imageViews[imageIndex],
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = vk::ClearValue{vk::ClearColorValue{std::array<float, 4>{
				-std::sin(time),
				std::cos(time),
				std::sin(time),
				1.0f,
			}}},
		};

		auto extent = window.framebufferSize();

		vk::RenderingInfo renderingInfo = {
			.renderArea = {.offset = {0, 0}, .extent = {extent.width, extent.height}},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
		};

		cmd.beginRendering(renderingInfo);
		cmd.endRendering();

		vk::ImageMemoryBarrier2 toPresent = {
			.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
			.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
			.dstAccessMask = vk::AccessFlags2{},
			.oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.newLayout = vk::ImageLayout::ePresentSrcKHR,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = images[imageIndex],
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
		};

		cmd.pipelineBarrier2(vk::DependencyInfo{
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &toPresent,
		});

		cmd.end();

		vk::SemaphoreSubmitInfo waitInfo = {
			.semaphore = *awaitAcquire[frame].vkSemaphore(),
			.stageMask = vk::PipelineStageFlagBits2::eClear,
		};

		vk::SemaphoreSubmitInfo signalInfo = {
			.semaphore = *signalRender[imageIndex].vkSemaphore(),
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
		// graphicsQueue.vkQueue().submit2({submitInfo});

		vk::PresentInfoKHR presentInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &*signalRender[imageIndex].vkSemaphore(),
			.swapchainCount = 1,
			.pSwapchains = &*swapchain.vkSwapchain(),
			.pImageIndices = &imageIndex,
		};

		try {
			auto presentResult = presentQueue.vkQueue().presentKHR(presentInfo);
		} catch (const vk::OutOfDateKHRError& error) {
			anvil::Swapchain newSwapchain{device, window, swapchainInfo, swapchain};
			swapchain = std::move(newSwapchain);

			images = swapchain.vkSwapchain().getImages();
			imageViews.clear();

			for (auto& image : images) {
				vk::ImageViewCreateInfo viewInfo = {
					.image = image,
					.viewType = vk::ImageViewType::e2D,
					.format = vk::Format::eB8G8R8A8Srgb,
					.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
				};

				imageViews.emplace_back(device.vkDevice(), viewInfo);
			}
		}

		frame = (frame + 1) % images.size();
		time += 0.01;

		frameCount++;
		auto now = std::chrono::steady_clock::now();
		float elapsed = std::chrono::duration<float>(now - lastTime).count();
		if (elapsed >= 1.0f) {
			std::printf("FPS: %d\n", frameCount);
			frameCount = 0;
			lastTime = now;
		}
	}

	device.vkDevice().waitIdle();
}
