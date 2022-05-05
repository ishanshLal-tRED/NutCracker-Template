import NutCracker.Example.VulkanExample;

#include <Core/Logging.hxx>

import <vulkan/vulkan.h>;
import <vector>;
import <set>;

import Nutcracker.Utils.Vulkan.DebugMarkers;

void NutCracker::Example::VulkanExample::initializeVk ()
{
	createInstance ();
	if (m_Vk.EnableDebugging)
		setupDebugging (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

	pickPhysicalDevice ();
	getEnabledFeatures ();

	createLogicalDevice (VK_QUEUE_GRAPHICS_BIT);
	getRequiredQueues (VK_QUEUE_GRAPHICS_BIT);

	createSyncronisationObjects ();

	prepare ();
}
void NutCracker::Example::VulkanExample::createInstance () {
	const auto& req_validation_layers = m_Vk.DefaultRequiredValidationLayers;
	auto& vk_extensions_to_enable = m_Vk.Static.EnabledInstanceExtensions;
	auto& vk_extensions_supported = m_Vk.Static.SuppotedExtensions;
	const bool enable_debugging = m_Vk.EnableDebugging;
	VkApplicationInfo app_info {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Vulkan Example",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};

	// these 2 types are mostly required by every vk instance (name can vary across OS(s))
	vk_extensions_to_enable.clear ();
	vk_extensions_to_enable.insert (VK_KHR_SURFACE_EXTENSION_NAME);
	#if NTKR_WINDOWS
		vk_extensions_to_enable.insert (VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	#endif
	
	{ // Vk Extensions
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties (nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions (extension_count);
		vkEnumerateInstanceExtensionProperties (nullptr, &extension_count, extensions.data ());
		
		vk_extensions_supported.clear ();
		vk_extensions_supported.reserve (extension_count)
		LOG_trace("available extensions:");
		for (const auto& extension : extensions) {
		    LOG_raw("\n\t {}", extension.extensionName);
			vk_extensions_supported.push_back (extension.extensionName);
		}
		LOG_trace("{} extensions supported", extension_count);
	}

	{ // Check validation layer support
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties (&layer_count, nullptr);
		std::vector<VkLayerProperties> validation_layers (layer_count);
		vkEnumerateInstanceLayerProperties (&layer_count, validation_layers.data ());

		std::vector<std::string_view> unavailable_layers;
		for (const char* layer_name : req_validation_layers) {
		    bool layerFound = false;
		
		    for (const auto& layer_properties : validation_layers) {
		        if (strcmp(layer_name, layer_properties.layerName) == 0) {
		            layerFound = true;
		            break;
		        }
		    }
		    if (!layerFound) {
		        unavailable_layers.push_back (layer_name);
		    }
		}
		if (!unavailable_layers.empty ()) {
			LOG_error("Missing Layer(s):");
			for (const char* extension_name: unavailable_layers)
				LOG_raw("\n\t {}", extension_name);
			THROW_CORE_critical("{:d} layer(s) not found", unavailable_layers.size ());
		}
	}
	
	VkInstanceCreateInfo create_info{
		// .pNext = (EnableValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info : nullptr), // to enable debugging while instance creation
		.pApplicationInfo = &app_info,
		.enabledLayerCount = uint32_t (enable_debugging ? std::size (req_validation_layers) : 0),
		.ppEnabledLayerNames = (enable_debugging ? &req_validation_layers[0] : nullptr),
		.enabledExtensionCount = uint32_t (vk_extensions_to_enable.size()),
		.ppEnabledExtensionNames = vk_extensions_to_enable.data()
	};

	if (vkCreateInstance(&create_info, nullptr, &m_Vk.Static.Instance) != VK_SUCCESS) 
		THROW_CORE_critical("failed to create instance!");
}
void NutCracker::Example::VulkanExample::createSurface ();
void NutCracker::Example::VulkanExample::setupDebugging (VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flags) {
	auto &instance = m_Vk.Static.Instance;
	auto &debug_messenger = m_Vk.Static.DebugMessenger;
	auto message_type_flags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	auto VKAPI_PTR *vk_debug_callback = [] (
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity
		, VkDebugUtilsMessageTypeFlagsEXT message_type
		, const VkDebugUtilsMessengerCallbackDataEXT* callback_data
		, void* pUserData) 
		{
			std::string msg_type;
			if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
				//if (!msg_type.empty())
				//	msg_type += ", ";
				msg_type += "VkGeneral";
			} 
			if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
				if (!msg_type.empty())
					msg_type += ", ";
				msg_type += "VkPerformance";
			} 
			if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
				if (!msg_type.empty())
					msg_type += ", ";
				msg_type += "VkValidation";
			}
			if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				LOG_error("{:s}: {:s}", msg_type, callback_data->pMessage); __debugbreak();
			} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
				LOG_warn("{:s}: {:s}", msg_type, callback_data->pMessage);
			} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
				LOG_info("{:s}: {:s}", msg_type, callback_data->pMessage);
			} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
				LOG_debug("{:s}: {:s}", msg_type, callback_data->pMessage);
			} else {
				LOG_trace("{:s}: {:s}", msg_type, callback_data->pMessage);
			}
		
			return VK_FALSE;
		};

	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info { // Reusing later when creating debug messenger
		.messageSeverity = message_severity_flags,
		.messageType = message_type_flags,
		.pfnUserCallback = vk_debug_callback,
		.pUserData = nullptr // Optional
	};

	VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) 
	    result = func(instance, &debug_messenger_info, nullptr, &debug_messenger);

	if (result != VK_SUCCESS)
		THROW_CORE_critical("failed setting up vk_debug_messenger");
};

void NutCracker::Example::VulkanExample::pickPhysicalDevice () {
	auto &selected_device = m_Vk.Static.PhysicalDevice;
	auto &selected_physical_mem_props = m_Vk.Static.PhysicalMemoryProperties;
	auto &selected_physical_properties = m_Vk.Static.PhysicalProperties;
	auto &selected_physical_features = m_Vk.Static.PhysicalFeatures;
	auto &selected_extensions = m_Vk.Static.EnabledDeviceExtensions; 
	auto &selected_queue_family_indices = m_Vk.Static.QueueFamilyIndices;
	auto &selected_surface_capabilities = m_Vk.Static.Surface.Capabilities;
	auto &selected_surface_formats = m_Vk.Static.Surface.Formats;
	auto &selected_surface_present_modes = m_Vk.Static.Surface.PresentModes;
	auto &selected_depth_format = m_Vk.Static.DepthFormat;
	const auto  surface = m_Vk.Static.Surface.Instance;
	const auto  instance = m_Vk.Static.Instance;
	const auto &req_extensions = m_Vk.DefaultRequiredDeviceExtensions;
	const auto &opt_extensions = m_Vk.OptionalDeviceExtensions;
	
	struct {
		std::set<std::string_view> enableable_extns; // All required extensions present

		uint32_t graphics_family, presentation_family, compute_family, transfer_family;

		VkSurfaceCapabilitiesKHR surface_capabilities;
		std::vector<VkSurfaceFormatKHR> surface_formats;
	    std::vector<VkPresentModeKHR> surface_present_modes;
	} cache;
	cache.enableable_extns.reserve (std::size (req_extensions) + std::size (opt_extensions));
	auto test_device_suitable = [&] (VkPhysicalDevice device) -> bool { 
		selected_physical_properties = {}, selected_physical_features = VkPhysicalDeviceFeatures{.samplerAnisotropy = VK_TRUE};
		vkGetPhysicalDeviceProperties (device, &selected_physical_properties);
		vkGetPhysicalDeviceFeatures (device, &selected_physical_features);
		
		cache.enableable_extns.clear ();
		cache.enableable_extns.insert (cache.enableable_extns.end (), std::begin (req_extensions), std::end (req_extensions));
		auto check_device_extensions_support = [&]() {
				uint32_t extension_count;
				vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, nullptr);
				std::vector<VkExtensionProperties> properties(extension_count);
				vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, properties.data());
				
				std::set<std::string_view> required_extensions (req_extensions);
				std::set<std::string_view> optional_extensions (opt_extensions);
			
				for(auto& extension: properties) {
					required_extensions.erase (extension.extensionName);
					for (auto opt_extn: optional_extensions) 
						if (opt_extn == extension.extensionName) {
							optional_extensions.erase (opt_extn);
							cache.enableable_extns.insert (opt_extn); // optional_extensions get string_vews indirectly from m_Vk.OptionalDeviceExtensions
							break;
						}
					if (required_extensions.empty () && optional_extensions.empty ()) break;
				}
				return required_extensions.empty();
			};

		constexpr auto uint32_max = std::numeric_limits<uint32_t>::max ();
		cache.graphics_family = uint32_max, cache.presentation_family = uint32_max;
		cache.compute_family = uint32_max, cache.transfer_family = uint32_max;
		auto get_queue_family_indices = [&]() {
				// Find required queue families
				uint32_t queue_family_count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
				std::vector<VkQueueFamilyProperties> queue_families (queue_family_count);
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

				bool dedicated_compute = false, dedicated_transfer = false;
				for (int i = 0; auto &queue_family: queue_families) {
					if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT && cache.graphics_family == uint32_max) // std::numeric_limits<uint32_t>::max () + 1 == 0 & !0 = 1
						cache.graphics_family = i;
					if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT  && (cache.compute_family == uint32_max || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && !dedicated_compute)
						cache.compute_family = i, dedicated_compute = (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;
					if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT && (cache.traansfer_family == uint32_max || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && !dedicated_transfer)
						cache.transfer_family = i, dedicated_transfer = (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;

					VkBool32 presentation_support_extension = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support_extension);
					if (presentation_support_extension)
						cache.presentation_family = i;

					if (!bool(cache.graphics_family + 1) && !bool(cache.presentation_family + 1) && !bool(cache.compute_family + 1) && !bool(cache.transfer_family + 1))
						return true;

					++i;
				}
				return !bool(cache.graphics_family + 1) && !bool(cache.presentation_family + 1); // we are confirming only graphics and presentation as mandatory
			};

		cache.surface_capabilities = {};
		cache.surface_formats.clear ();
		cache.surface_present_modes.clear ();
		auto query_swap_chain_support = [&]() {
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &cache.surface_capabilities);
				
				uint32_t format_count;
				vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &format_count, nullptr);
				if (format_count != 0) {
					cache.surface_formats.resize(format_count);
					vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &format_count, cache.surface_formats.data());
				}
				
				uint32_t present_mode_count;
				vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &present_mode_count, nullptr);
				if (present_mode_count != 0) {
					cache.surface_present_modes.resize(present_mode_count);
					vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &present_mode_count, cache.surface_present_modes.data ());
				}
			}

		if (check_device_extensions_support (device) && get_queue_family_indices (device)) {
			query_swap_chain_support ();
			if (!cache.surface_formats.empty () && !cache.surface_present_modes.empty ()) {
				return true
					&& selected_physical_features.geometryShader
					&& selected_physical_features.samplerAnisotropy
				;
			}
		}
	};

	enabled_extensions.clear ();
	enabled_extensions.reserve (std::size (req_extensions));

	selected_device = VK_NULL_HANDLE;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0)
		THROW_CORE_critical("failed to find hardware with vulkan support");
	std::vector<VkPhysicalDevice> vk_enabled_devices (device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, vk_enabled_devices.data ());

	for (auto device: vk_enabled_devices) {
		if (test_device_suitable (device)) { // 
			selected_device = device; 
			selected_physical_properties;
			selected_physical_features;
			selected_extensions = std::move (cache.enableable_extns);
			selected_queue_family_indices.Graphics = cache.graphics_family;
			selected_queue_family_indices.Presentation = cache.presentation_family;
			selected_queue_family_indices.Compute = cache.compute_family;
			selected_queue_family_indices.Transfer = cache.transfer_family;
			selected_surface_capabilities = cache.surface_capabilities;
			selected_surface_formats = cache.surface_formats;
			selected_surface_present_modes = cache.surface_present_modes;
			if (selected_physical_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				break; // iGPUs are good and all, but descrete ones are better
		}
	}
	if (selected_device == VK_NULL_HANDLE) 
		THROW_CORE_critical("failed to find suitable hardware with required vulkan support");

	vkGetPhysicalDeviceMemoryProperties (selected_device, &selected_physical_mem_props);
	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format
	VkFormat depth_formats[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};
	for (auto& format : depth_formats) {
		VkFormatProperties format_props;
		vkGetPhysicalDeviceFormatProperties(selected_device, format, &format_props);
		// Format must support depth stencil attachment for optimal tiling
		if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			selected_depth_format = format;
	}
}
void NutCracker::Example::VulkanExample::getEnabledFeatures () {
	m_Vk.Dynamic.EnabledFeatures = m_Vk.Static.PhysicalFeatures; // All features

	if (m_Vk.Static.Surface.Capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) 
		m_Vk.Static.Surface.Capabilities.currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
}
void NutCracker::Example::VulkanExample::createLogicalDevice (const VkQueueFlagBits queue_types) {
	auto& logical_device  = m_Vk.Dynamic.LogicalDevice;
	const auto  physical_device = m_Vk.Static.PhysicalDevice;
	const auto& enabled_features = m_Vk.Dynamic.EnabledFeatures;
	const auto& enabled_extensions = m_Vk.Static.EnabledDeviceExtensions;
	const auto& enabled_validation_layers = m_Vk.DefaultRequiredValidationLayers;
	const bool  enable_debugging = m_Vk.EnableDebugging;

	std::set<uint32_t> unique_queue_families;

	if (queue_types & VK_QUEUE_GRAPHICS_BIT || true) {
		unique_queue_families.insert (m_Vk.Static.Graphics); 
		unique_queue_families.insert (m_Vk.Static.Presentation);
	}
	if (queue_types & VK_QUEUE_COMPUTE_BIT)
		unique_queue_families.insert (m_Vk.Static.Compute);
	if (queue_types & VK_QUEUE_TRANSFER_BIT)
		unique_queue_families.insert (m_Vk.Static.Transfer);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	queue_create_infos.reserve (unique_queue_families.size());
	for (auto family_indice: unique_queue_families) {
		float queue_priority = 1.0f; // need more information on it
		queue_create_infos.push_back(VkDeviceQueueCreateInfo {
			.queueFamilyIndex = family_indice,
			.queueCount = 1,
			.pQueuePriorities = &queue_priority
		});
	}

	VkDeviceCreateInfo create_info {
		.queueCreateInfoCount = uint32_t(queue_create_infos.size ()),
		.pQueueCreateInfos = queue_create_infos.data (),
		.enabledLayerCount = uint32_t (enable_debugging ? std::size (enabled_validation_layers) : 0),
		.ppEnabledLayerNames = (enable_debugging ? &enabled_validation_layers[0] : nullptr),
		.enabledExtensionCount = uint32_t(std::size (enabled_extensions)),
		.ppEnabledExtensionNames = &enabled_extensions[0],
		.pEnabledFeatures = &enabled_features,
	};

	if (vkCreateDevice (physical_device, &create_info, nullptr, &logical_device) != VK_SUCCESS) 
		THROW_CORE_critical ("Failed to create logical device");
}
 // can be(s) graphics, presentation, compute, transfer VkQuees
void NutCracker::Example::VulkanExample::getRequiredQueues () {
	auto& logical_device  = m_Vk.Dynamic.LogicalDevice;
	auto& queues  = m_Vk.Static.QueueFamilyIndexes;
	for (int i = 0; uint32_t queue_family: m_Vk.Static.QueueFamilyIndexes) {
		if (queue_family != std::numeric_limits<uint32_t>::max ())
			vkGetDeviceQueue (logical_device, queue_family, 0, queues[i]);
		++i;
	}
}

void createSyncronisationObjects () {
	// Create syncronisation objects
	VkSemaphoreCreateInfo semaphore_info {};

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if ( vkCreateSemaphore (m_Context.Vk.LogicalDevice, &semaphore_info, nullptr, &m_Context.Vk.ImageAvailableSemaphores[i]) != VK_SUCCESS
		  || vkCreateSemaphore (m_Context.Vk.LogicalDevice, &semaphore_info, nullptr, &m_Context.Vk.RenderFinishedSemaphores[i]) != VK_SUCCESS)
			THROW_Critical ("failed to create semaphores!");
	}	
}

void NutCracker::Example::VulkanExample::prepare () {

	// The VK_EXT_debug_marker extension is a device extension. 
	// It introduces concepts of object naming and tagging, for better tracking of {apiname} objects, 
	// as well as additional Commands for recording annotations of named sections of a workload to aid 
	// organisation and offline analysis in external tools.
	if (m_Vk.EnableDebugMarkers) 
		NutCracker::DebugMarker::setup  (m_Vk.Dynamic.LogicalDevice);
	
	initSwapchain();
	createCommandPool (VK_QUEUE_GRAPHICS_BIT);
	setupSwapChain();
	createCommandBuffers();
	createSynchronizationPrimitives();
	setupDepthStencil();
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();
};

void NutCracker::Example::VulkanExample::initSwapchain () {
	auto &selected_color_format = m_Vk.Dynamic.Surface.ImageFormat; 
	auto &selected_color_space = m_Vk.Dynamic.Surface.ColorSpace; 
	auto &surface_formats = m_Vk.Static.Surface.Formats; 
	// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
	// there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
	if ((std::size (surface_formats) == 1) && (surface_formats[0].format == VK_FORMAT_UNDEFINED))
	{
		selected_color_format = VK_FORMAT_B8G8R8A8_SRGB;
		selected_color_space = surface_formats[0].colorSpace;
	}
	else
	{
		// iterate over the list of available surface format and
		// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
		bool found_B8G8R8A8_SRGB = false;
		for (auto&& surface_format : surface_formats)
		{
			if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				selected_color_format = surface_format.format;
				selected_color_space = surface_format.colorSpace;
				found_B8G8R8A8_SRGB = true;
				break;
			}
		}

		// in case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		if (!found_B8G8R8A8_SRGB)
		{
			selected_color_format = surface_formats[0].format;
			selected_color_space = surface_formats[0].colorSpace;
		}
	}
}
// currently only for graphics
void NutCracker::Example::VulkanExample::createCommandPool (const VkQueueFlagBits pool_for_queue_types) {
	auto &cmd_pool = m_Vk.Dynamic.CmdPool;
	const auto device = m_Vk.Dynamic.LogicalDevice;
	const auto graphics_family = m_Vk.Static.Queues.Graphics;
//	const auto compute_family = m_Vk.Static.Queues.Compute;
//	const auto transfer_family = m_Vk.Static.Queues.Transfer;

	std::vector<uint32_t> pool_of_queue_types;
	if (pool_for_queue_types & VK_QUEUE_GRAPHICS_BIT && graphics_family != uint32_max)
		pool_of_queue_types.push_back (graphics_family);
//	if (pool_for_queue_types & VK_QUEUE_COMPUTE_BIT  && compute_family  != uint32_max)
//		pool_of_queue_types.push_back (compute_family);
//	if (pool_for_queue_types & VK_QUEUE_TRANSFER_BIT && transfer_family != uint32_max)
//		pool_of_queue_types.push_back (transfer_family);

	if (pool_of_queue_types.empty ())
		THROW_critical ("no pool to create");

	VkCommandPoolCreateInfo pool_info {
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = pool_of_queue_types [0],
	};

	if (vkCreateCommandPool (device, &pool_info, nullptr, &cmd_pool) != VK_SUCCESS) 
		THROW_CORE_critical ("failed to create command pool!");
}
void NutCracker::Example::VulkanExample::setupSwapChain () {
	auto  old_swapchain = std::move (m_Vk.Dynamic.Swapchain); // saving the old
	auto &new_swapchain = m_Vk.Dynamic.Swapchain;
	const auto [fb_width, fb_height] = m_Window.GetFramebufferSize ();
	const auto  surface = m_Vk.Static.Surface.Instance;
	const auto& surface_capabilities_supported = m_Vk.Static.Surface.Capabilities;
	const auto& surface_present_mode_supported = m_Vk.Static.Surface.PresentModes;
	const auto graphics_family_indice = m_Vk.Static.QueueFamilyIndices.Graphics, presentation_family_indice = m_Vk.Static.QueueFamilyIndices.Presentation;
	const auto logical_device = m_Vk.Dynamic.LogicalDevice;
	const auto save_swapchin_colorspace = old_swapchain.ColorSpace;
	const auto save_swapchin_format = old_swapchain.ImageFormat;

	// choose swap presentation mode
	VkExtent2D extent_selected;
	VkPresentModeKHR present_mode_selected = VK_PRESENT_MODE_FIFO_KHR; // default
	for (const auto& present_mode: surface_present_mode_supported) {
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode_selected = present_mode;
			break;
		}
		if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			present_mode_selected = present_mode;
			break;
		}
	}
	if (surface_capabilities_supported.currentExtent.width != std::numeric_limits<uint32_t>::max ()) {
		extent_selected = surface_capabilities_supported.currentExtent;
	} else {
		extent_selected = VkExtent2D {
			.width  = std::clamp (fb_width , surface_capabilities_supported.minImageExtent.width , surface_capabilities_supported.maxImageExtent.width),
			.height = std::clamp (fb_height, surface_capabilities_supported.minImageExtent.height, surface_capabilities_supported.maxImageExtent.height)
		};
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR composite_alpha;
	// Simply select the first composite alpha format available
	VkCompositeAlphaFlagBitsKHR composite_alpha_flags[] = { // order of priority
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& composite_alpha_flag : composite_alpha_flags) {
		if (surface_capabilities_supported.supportedCompositeAlpha & composite_alpha_flag) {
			composite_alpha = composite_alpha_flag;
			break;
		};
	}

	uint32_t image_count = ((surface_capabilities_supported.maxImageCount == 0) ? surface_capabilities_supported.minImageCount + 1 : surface_capabilities_supported.maxImageCount);
	VkSwapchainCreateInfoKHR swapchain_create_info {
		.surface = surface,
		.minImageCount = image_count,
		.imageFormat = save_swapchin_format,
		.imageColorSpace = save_swapchin_colorspace,
		.imageExtent = extent_selected,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

		.preTransform = surface_capabilities_supported.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode_selected,
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		.clipped = VK_TRUE,
		// Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
		.oldSwapchain = old_swapchain.Instance,
	};

	// Enable transfer source on swap chain images if supported
	if (surface_capabilities_supported.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	// Enable transfer destination on swap chain images if supported
	if (surface_capabilities_supported.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (graphics_family_indice == presentation_family_indice) {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_create_info.queueFamilyIndexCount = 0;
		swapchain_create_info.pQueueFamilyIndices = nullptr;
	} else {
		uint32_t queue_family_indices[] = {graphics_family_indice, presentation_family_indice};
		
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = uint32_t(std::size(queue_family_indices));
		swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
	}

	if (vkCreateSwapchainKHR (logical_device, &swapchain_create_info, nullptr, &new_swapchain.Instance) != VK_SUCCESS)
		THROW_critical("failed to create swapchain !");

	// cleanup old swapchain
	if (old_swapchain.Instance != VK_NULL_HANDLE) {
		for (auto &image_view: old_swapchain.ImagesView) 
			vkDestroyImageView (logical_device, image_view, nullptr);
		vkDestroySwapchainKHR (logical_device, old_swapchain.Instance, nullptr);
	}
	new_swapchain.ImagesView = std::move (old_swapchain.ImagesView);
	new_swapchain.Images = std::move (old_swapchain.Images);

	vkGetSwapchainImagesKHR(logical_device, new_swapchain.Instance, &image_count, nullptr);
	new_swapchain.ImagesView.resize (image_count);
	new_swapchain.Images.resize (image_count);
	vkGetSwapchainImagesKHR(logical_device, new_swapchain.Instance, &image_count, new_swapchain.Images.data ());
	new_swapchain.ImageExtent = extent_selected;
	new_swapchain.ImageFormat = save_swapchin_format;
	new_swapchain.ColorSpace  = save_swapchin_colorspace;

	for (size_t i = 0; i < image_count; ++i) {
		VkImageViewCreateInfo create_info {
			.image = new_swapchain.Images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = new_swapchain.ImageFormat,
			// equivalent to {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A}
			.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			.subresourceRange = VkImageSubresourceRange {
				    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				    .baseMipLevel = 0,
				    .levelCount = 1,
				    .baseArrayLayer = 0,
				    .layerCount = 1
			}
		};

		if (vkCreateImageView (logical_device, &create_info, nullptr, &new_swapchain.ImagesView[i]) != VK_SUCCESS)
			THROW_critical ("failed to create imagfe views!");
	}
}
void NutCracker::Example::VulkanExample::createCommandBuffers () {
	// Create command buffer
	VkCommandBufferAllocateInfo alloc_info {
		.commandPool = m_Vk.Dynamic.CmdPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = uint32_t (std::size (m_Vk.Dynamic.DrawCmdBuffers)),
	};
	
	if (vkAllocateCommandBuffers(m_Vk.Dynamic.LogicalDevice, &alloc_info, &m_Vk.Dynamic.DrawCmdBuffers[0]) != VK_SUCCESS) 
	    THROW_CORE_critical ("failed to allocate command buffers!");
}
void NutCracker::Example::VulkanExample::createSynchronizationPrimitives () {
	// Wait fences to sync command buffer access
	VkFenceCreateInfo fence_info {
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	static_assert(std::size (m_Vk.Dynamic.InFlightFences) == std::size (m_Vk.Dynamic.DrawCmdBuffers));

	for (auto& fence : m_Vk.Dynamic.InFlightFences) 
		if (vkCreateFence(m_Vk.Dynamic.LogicalDevice, &fence_info, nullptr, &fence) != VK_SUCCESS)
			THROW_critical ("failed to create fence");
	
}

uint32_t NutCracker::Example::VulkanExample::findMemoryType (uint32_t type_filter, VkMemoryPropertyFlags properties) {
	auto physical_device = m_Vk.Static.PhysicalDevice;
	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
		if ( (type_filter & (1 << i)) && 
			((mem_properties.memoryTypes[i].propertyFlags & properties) == properties) )
			return i;
	
	THROW_Critical ("failed to find suitable memory type");
}

void NutCracker::Example::VulkanExample::setupDepthStencil () {
	auto&  depth_stencil = m_Vk.Dynamic.DepthStencil;
	const auto device = m_Vk.Dyanmic.LogicalDevice;
	const auto depth_format = m_Vk.Static.DepthFormat;
	const auto image_extent = m_Vk.Dynamic.Swapchain.ImageExtent;
	if (depth_format == VK_FORMAT_UNDEFINED) 
		return;
	
	VkImageCreateInfo image_info {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depth_format,
		.extent = { image_extent.Width, image_extent.Height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	};
	
	if (vkCreateImage(device, &image_info, nullptr, &depth_stencil.Image))
		THROW_CORE_critical ("unable to create depth image");
	VkMemoryRequirements mem_reqs{};
	vkGetImageMemoryRequirements(device, depth_stencil.Image, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc_info {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = findMemoryType (mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};
	if (vkAllocateMemory(device, &mem_alloc_info, nullptr, &depth_stencil.ImageMem) != VK_SUCCESS)
		THROW_critical ("cannot allocate image memory to depth stencil");
	if (vkBindImageMemory(device, depth_stencil.Image, depth_stencil.ImageMem, 0) != VK_SUCCESS)
		THROW_critical ("cannot bind image memory to depth stencil image");

	VkImageViewCreateInfo image_view_info {
		.image = depth_stencil.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depth_format,
		.subresourceRange = VkImageSubresourceRange {
			    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			    .baseMipLevel = 0,
			    .levelCount = 1,
			    .baseArrayLayer = 0,
			    .layerCount = 1,
		},
	};
	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
	if (depth_format >= VK_FORMAT_D16_UNORM_S8_UINT) 
		image_view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	
	if (vkCreateImageView(device, &image_view_info, nullptr, &depth_stencil.ImageView) != VK_SUCCESS) 
		THROW_critical ("failed to create depth image");
	
}
void NutCracker::Example::VulkanExample::setupRenderPass (bool add_depth_stencil_to_attachment) {
	auto& render_pass = m_Vk.Dynamic.RenderPass;
	auto& depth_format = m_Vk.Static.DepthFormat;
	const auto& swapchain = m_Vk.Dynamic.Swapchain;

	std::vector<VkAttachmentDescription> attachments; // for color pass and depth pass
	std::vector<VkAttachmentReference> color_attachments_ref;
	VkAttachmentDescription depth_attachment;
	VkAttachmentReference depth_attachment_ref;

	VkSubpassDescription subpass_description {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 0,
		.pColorAttachments = nullptr,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr,
	};

	{ // color attachment #1 
		uint32_t counter = attachments.size ();
		attachments.push_back (VkAttachmentDescription {
			.format = swapchain.ImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		});
		color_attachments_ref.push_bach (VkAttachmentReference {
			.attachment = counter,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		});
	} // ... likewise all desired color attachmens
	

	if (add_depth_stencil_to_attachment) {
		if (depth_format == VK_FORMAT_UNDEFINED) 
			THROW_critical ("no depth format to create attachment out of.");

		uint32_t counter = attachments.size ();
		attachments.push_back (VkAttachmentDescription {
			.format = depth_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		});
		depth_attachment_ref = VkAttachmentReference {
			.attachment = counter,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};
	}

	subpass_description.pDepthStencilAttachment = &depth_attachment_ref;
	subpass_description.colorAttachmentCount = color_attachments_ref.size ();
	subpass_description.pColorAttachments = color_attachments_ref.data ();

	

}
void NutCracker::Example::VulkanExample::createPipelineCache () {
}
void NutCracker::Example::VulkanExample::setupFrameBuffer () {
}


void NutCracker::Example::VulkanExample::submitFrame () {
}
void NutCracker::Example::VulkanExample::render(double render_latency) {
}

void NutCracker::Example::VulkanExample::cleanupSwapchain () {
}
void NutCracker::Example::VulkanExample::destroyCommandBuffers () {
}
void NutCracker::Example::VulkanExample::terminateVk() {
}


#if NTKR_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>; // re include for left overones
void NutCracker::Example::VulkanExample::createSurface () {
	auto& surface = m_Vk.Static.Surface.Instance;
	auto& instance = m_Vk.Static.Instance;
	#if NTKR_WINDOWS
		VkWin32SurfaceCreateInfoKHR surface_info{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle (nullptr),
			.hwnd = HWND(m_Window->GetNativeWindow ()),
		};
		if (vkCreateWin32SurfaceKHR (instance, &surface_info, nullptr, &surface) != VK_SUCCESS) 
			THROW_CORE_critical("failed to create window surface");
	#endif
}