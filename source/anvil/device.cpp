#include <anvil/device.hpp>
#include <anvil/physical_device.hpp>
#include <anvil/window.hpp>

using namespace anvil;

Device::Device(PhysicalDevice& physicalDevice, Window& sampleWindow, const DeviceQueueCounts& queueCounts, const DeviceFeatures& features)
	: physicalDevice_(&physicalDevice), features_(features), vkDevice_(makeDevice(sampleWindow, queueCounts)) {
}

vk::raii::Device& Device::vkDevice() {
	return vkDevice_;
}

const vk::raii::Device& Device::vkDevice() const {
	return vkDevice_;
}

PhysicalDevice& Device::physicalDevice() {
	return *physicalDevice_;
}

const PhysicalDevice& Device::physicalDevice() const {
	return *physicalDevice_;
}

Queue Device::getQueue(const QueueBinding& binding) const {
	QueueDefinition definition;

	switch (binding.usage) {
		case QueueUsage::Graphics: {
			definition = graphicsDefinitions_[binding.index];

			break;
		}
		case QueueUsage::Present: {
			definition = presentDefinitions_[binding.index];

			break;
		}
		case QueueUsage::Transfer: {
			definition = transferDefinitions_[binding.index];

			break;
		}
		case QueueUsage::Compute: {
			definition = computeDefinitions_[binding.index];

			break;
		}
	}

	return {vkDevice_.getQueue(definition.familyIndex, definition.queueIndex)};
}

vk::raii::Device Device::makeDevice(Window& sampleWindow, const DeviceQueueCounts& queueCounts) {
	auto& physicalDevice = physicalDevice_->vkPhysicalDevice();
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

	static constexpr const char* PortabilitySubset = "VK_KHR_portability_subset";

	auto findSubset = [&](const auto& candidate) {
		return candidate.extensionName == std::string_view(PortabilitySubset);
	};

	if (std::find_if(deviceExtensions.begin(), deviceExtensions.end(), findSubset) != deviceExtensions.end()) {
		requiredDeviceExtensions.emplace_back("VK_KHR_portability_subset");
	}

	auto queueFamilies = physicalDevice.getQueueFamilyProperties();

	std::vector<std::vector<float>> priorities;
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;

	priorities.reserve(queueFamilies.size());

	DeviceQueueCounts locatedCounts = {
		.graphics = 0,
		.present = 0,
		.transfer = 0,
		.compute = 0,
	};

	for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
		auto& family = queueFamilies[i];

		bool graphicsCapable = (family.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics;
		bool presentCapable = physicalDevice.getSurfaceSupportKHR(i, sampleWindow.vkSurface());
		bool transferCapable = (family.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer;
		bool computeCapable = (family.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute;

		bool acceptsGraphics = locatedCounts.graphics < queueCounts.graphics;
		bool acceptsPresent = locatedCounts.present < queueCounts.present;
		bool acceptsTransfer = locatedCounts.transfer < queueCounts.transfer;
		bool acceptsCompute = locatedCounts.compute < queueCounts.compute;

		bool acceptsAny = acceptsGraphics || acceptsPresent || acceptsTransfer || acceptsCompute;

		if (!acceptsAny) {
			continue;
		}

		auto& queueInfo = queueInfos.emplace_back();
		auto& familyPriorities = priorities.emplace_back();

		auto tryUse = [&](bool condition, auto& list, auto& count) {
			if (acceptsGraphics && graphicsCapable) {
				++count;

				bool mustAlias = queueInfo.queueCount == family.queueCount;

				if (!mustAlias) {
					++queueInfo.queueCount;
					familyPriorities.emplace_back(1.0);
				}

				list.emplace_back(i, queueInfo.queueCount - 1);
			}
		};

		tryUse(acceptsGraphics && graphicsCapable, graphicsDefinitions_, locatedCounts.graphics);
		tryUse(acceptsPresent && presentCapable, presentDefinitions_, locatedCounts.present);
		tryUse(acceptsTransfer && transferCapable, transferDefinitions_, locatedCounts.transfer);
		tryUse(acceptsCompute && computeCapable, computeDefinitions_, locatedCounts.compute);

		queueInfo.queueFamilyIndex = i;
		queueInfo.pQueuePriorities = familyPriorities.data();
	}

	vk::PhysicalDeviceVulkan13Features vulkan13Features = {
		.synchronization2 = true,
		.dynamicRendering = true,
	};

	vk::DeviceCreateInfo deviceInfo = {
		.pNext = &vulkan13Features,
		.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
		.pQueueCreateInfos = queueInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
		.ppEnabledExtensionNames = requiredDeviceExtensions.data(),
	};

	return {physicalDevice, deviceInfo};
}
