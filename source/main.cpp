#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Space Game", nullptr, nullptr);

    {
        vk::raii::Context context;

        auto supportedLayers = context.enumerateInstanceLayerProperties();
        auto supportedExtensions = context.enumerateInstanceExtensionProperties();
        auto apiVersion = context.enumerateInstanceVersion();

        std::vector<const char*> requiredLayers = {
            "VK_LAYER_KHRONOS_validation",
        };

        for (auto& requirement : requiredLayers) {
            auto condition = [&](const auto& candidate) {
                return candidate.layerName == std::string_view(requirement);
            };

            auto candidate = std::find_if(supportedLayers.begin(), supportedLayers.end(), condition);

            if (candidate == supportedLayers.end()) {
                throw std::runtime_error("required layer is missing!");
            }
        }

        uint32_t windowExtensionCount = 0;

        const char** windowExtensions = glfwGetRequiredInstanceExtensions(&windowExtensionCount);

        std::vector<const char*> requiredExtensions{windowExtensions, windowExtensions + windowExtensionCount};

        for (auto& requirement : requiredExtensions) {
            auto condition = [&](const auto& candidate) {
                return candidate.extensionName == std::string_view(requirement);
            };

            auto candidate = std::find_if(supportedExtensions.begin(), supportedExtensions.end(), condition);

            if (candidate == supportedExtensions.end()) {
                throw std::runtime_error("required extension is missing!");
            }
        }

        vk::ApplicationInfo applicationInfo = {
            .pApplicationName = "space game",
            .applicationVersion = vk::makeVersion(0, 1, 0),
            .pEngineName = "anvil",
            .engineVersion = vk::makeVersion(0, 1, 0),
            .apiVersion = apiVersion,
        };

        vk::InstanceCreateInfo instanceInfo = {
            .flags = vk::InstanceCreateFlags{},
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        vk::raii::Instance instance{context, instanceInfo};

        auto physicalDevices = instance.enumeratePhysicalDevices();
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

        std::vector<float> priorities = { 1.0 };

        vk::DeviceQueueCreateInfo queueInfo = {
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = priorities.data(),
        };

        vk::DeviceCreateInfo deviceInfo = {
            .flags = vk::DeviceCreateFlags{},
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueInfo,
            .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
        };

        vk::raii::Device device{physicalDevice, deviceInfo};
        vk::raii::Queue queue = device.getQueue(0, 0);

        VkSurfaceKHR vkSurface;

        glfwCreateWindowSurface(*instance, window, nullptr, &vkSurface);

        vk::raii::SurfaceKHR surface{instance, vkSurface};

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

        size_t frame = 0;
        float time = 0.0;

        while (!glfwWindowShouldClose(window)) {
            time += 0.001;
            glfwPollEvents();

            auto& fence = inFlight[frame];

            auto waitResult = device.waitForFences({fence}, true, UINT64_MAX);
            device.resetFences({fence});

            auto [acquireResult, imageIndex] = swapchain.acquireNextImage(UINT64_MAX, awaitAcquire[frame]);

            auto& cmd = commandBuffers[frame];

            cmd.reset();
            cmd.begin(vk::CommandBufferBeginInfo{});

            vk::ImageMemoryBarrier toClear = {
                .srcAccessMask = vk::AccessFlags{},
                .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .image = images[imageIndex],
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
            };

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {toClear});

            vk::ClearColorValue clearColor{std::array<float,4>{
                -std::sin(time),
                std::cos(time),
                std::sin(time),
                1.0f,
            }};

            cmd.clearColorImage(images[imageIndex], vk::ImageLayout::eTransferDstOptimal, clearColor, {vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

            vk::ImageMemoryBarrier toPresent = {
                .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask = vk::AccessFlags{},
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = vk::ImageLayout::ePresentSrcKHR,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .image = images[imageIndex],
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
            };

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, {toPresent});

            cmd.end();

            vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTransfer;
            vk::SubmitInfo submitInfo = {
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*awaitAcquire[frame],
                .pWaitDstStageMask = &waitStage,
                .commandBufferCount = 1,
                .pCommandBuffers = &*cmd,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &*signalRender[frame],
            };

            queue.submit({submitInfo}, fence);

            vk::PresentInfoKHR presentInfo = {
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*signalRender[frame],
                .swapchainCount = 1,
                .pSwapchains = &*swapchain,
                .pImageIndices = &imageIndex,
            };

            auto presentResult = queue.presentKHR(presentInfo);

            frame = (frame + 1) % images.size();
        }

        device.waitIdle();
    }

    glfwTerminate();
}
