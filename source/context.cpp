#include <context.hpp>

using namespace anvil;

Context::Context(const ApplicationInfo& applicationInfo, const ContextFeatures& features)
	: applicationInfo_(applicationInfo), features_(features), vkInstance_(makeInstance()) {
}

glfw::Context& Context::glfwContext() {
	return glfwContext_;
}

const glfw::Context& Context::glfwContext() const {
	return glfwContext_;
}

vk::raii::Context& Context::vkContext() {
	return vkContext_;
}

const vk::raii::Context& Context::vkContext() const {
	return vkContext_;
}

vk::raii::Instance& Context::vkInstance() {
	return vkInstance_;
}

const vk::raii::Instance& Context::vkInstance() const {
	return vkInstance_;
}

vk::raii::Instance Context::makeInstance() {
	auto supportedLayers = vkContext_.enumerateInstanceLayerProperties();
	auto supportedExtensions = vkContext_.enumerateInstanceExtensionProperties();
	auto apiVersion = vkContext_.enumerateInstanceVersion();

	static constexpr const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

	std::vector<const char*> requiredLayers;

	if (features_.debugging) {
		requiredLayers.emplace_back(validationLayerName);
	}

	for (auto& requirement : requiredLayers) {
		auto condition = [&](const auto& candidate) {
			return candidate.layerName == std::string_view(requirement);
		};

		auto candidate = std::find_if(supportedLayers.begin(), supportedLayers.end(), condition);

		if (candidate == supportedLayers.end()) {
			throw std::runtime_error("required instance layer is missing!");
		}
	}

	std::vector<const char*> requiredExtensions = glfwContext_.requiredVulkanExtensions();

	auto findPortability = [&](const auto& candidate) {
		return candidate.extensionName == std::string_view(vk::KHRPortabilityEnumerationExtensionName);
	};

	bool portable = std::find_if(supportedExtensions.begin(), supportedExtensions.end(), findPortability) != supportedExtensions.end();

	if (portable) {
		requiredExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
	}

	for (auto& requirement : requiredExtensions) {
		auto condition = [&](const auto& candidate) {
			return candidate.extensionName == std::string_view(requirement);
		};

		auto candidate = std::find_if(supportedExtensions.begin(), supportedExtensions.end(), condition);

		if (candidate == supportedExtensions.end()) {
			throw std::runtime_error("required instance extension is missing!");
		}
	}

	vk::ApplicationInfo applicationInfo = {
		.pApplicationName = applicationInfo_.name.data(),
		.applicationVersion = vk::makeVersion(applicationInfo_.version.major, applicationInfo_.version.minor, applicationInfo_.version.patch),
		.pEngineName = "anvil",
		.engineVersion = vk::makeVersion(0, 1, 0),
		.apiVersion = apiVersion,
	};

	vk::InstanceCreateInfo instanceInfo = {
		.flags = portable ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR : vk::InstanceCreateFlags{},
		.pApplicationInfo = &applicationInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
		.ppEnabledExtensionNames = requiredExtensions.data(),
	};

	return {vkContext_, instanceInfo};
}
