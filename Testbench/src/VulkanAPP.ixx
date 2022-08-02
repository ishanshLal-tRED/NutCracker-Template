#include <Nutcracker/Core/Logging.hxx>;
#include <Nutcracker/Utils/HelperMacros.hxx>;

#if NTKR_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#undef max
#undef min

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

import <iostream>;
import <fstream>;
import <stdexcept>;
import <algorithm>;
import <chrono>;
import <vector>;
import <cstring>;
import <cstdlib>;
import <cstdint>;
import <limits>;
import <array>;
import <optional>;
import <set>;

import NutCracker.Base.AppInstance;
import NutCracker.Base.Window;
import NutCracker.Events;
import Nutcracker.Utils.Vulkan.DebugMarkers;
import Nutcracker.Utils.Assets;

export module NutCracker.Example.VulkanAPP;
import :Context;

#define VK_ASSERTN(x,e,...)	if (VkResult r = x; r != VK_SUCCESS) THROW_critical ("VK_ASSERT failed: CALLER {} {}", std::source_location::current ().function_name (), fmt::format(e, __VA_ARGS__))
#define VK_ASSERT1(x)		if (VkResult r = x; r != VK_SUCCESS) THROW_critical ("VK_ASSERT failed: CALLER {}", std::source_location::current ().function_name ())
#define VK_ASSERT2(x,e)		if (VkResult r = x; r != VK_SUCCESS) THROW_critical ("VK_ASSERT failed: CALLER {} {}", std::source_location::current ().function_name (), e)
#define VK_ASSERT3(x,e,...)	VK_ASSERTN(x,e,__VA_ARGS__)
#define VK_ASSERT4(x,e,...)	VK_ASSERTN(x,e,__VA_ARGS__)
#define VK_ASSERT5(x,e,...)	VK_ASSERTN(x,e,__VA_ARGS__)

#define VK_ASSERT(...)		{ NTKR_VMACRO(VK_ASSERT, __VA_ARGS__) }

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::array<Vertex, 4> s_QuadVertices = {
	Vertex{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	Vertex{{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	Vertex{{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
	Vertex{{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::array<uint16_t, 6> s_QuadIndices = {
	0, 1, 2, 2, 3, 0
};

namespace NutCracker::Example {
	export
	class VulkanAPP: public ::NutCracker::BaseInstance {
	private:
		virtual void setup (const std::span<const char*> argument_list) override {
			LOG_trace (std::source_location::current ().function_name ());
			m_Window = Window::Create (WindowProps {
				.Title = "Vulkan Sandbox 1",
				.Width = 1280,
				.Height = 720,
			});
			m_Window->SetEventCallback (NTKR_BIND_FUNCTION (onEvent));
		}
		virtual void initializeVk () override {
			createInstance();
			if (m_Vk.EnableDebugging)
				setupDebugging(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

			createSurface();

			pickPhysicalDevice();

			setDevicePrefrences();
			constexpr auto queue_types = VkQueueFlagBits(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
			createLogicalDevice (queue_types);
			getRequiredQueues (queue_types);

			prepare ();
		}
		bool onWndClose (WindowCloseEvent& e) {
			m_Running = false;
			return true;
		}
		bool onWndResize (WindowResizeEvent& e) {
			m_FramebufferResized = true;
			return true;
		}
		virtual bool keepContextRunning () override { return m_Running; }
		virtual void onEvent (Event& e) override {
			EventDispatcher dispatcher (e);
			dispatcher.Dispatch<WindowCloseEvent> (NTKR_BIND_FUNCTION (onWndClose));
			dispatcher.Dispatch<WindowResizeEvent> (NTKR_BIND_FUNCTION (onWndResize));
		}
//// TODO
		virtual void update (double update_latency) override {
			glfwPollEvents();
		}
		virtual void render (double render_latency) override {
			vkWaitForFences(m_Vk.Dynamic.LogicalDevice, 1, &m_Vk.Dynamic.InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

			uint32_t imageIndex;
			VkResult result = vkAcquireNextImageKHR(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.Swapchain.Instance, UINT64_MAX, m_Vk.Dynamic.ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				recreateSwapChain();
				return;
			} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				throw std::runtime_error("failed to acquire swap chain image!");
			}

			updateUniformBuffer(m_CurrentFrame);

			vkResetFences(m_Vk.Dynamic.LogicalDevice, 1, &m_Vk.Dynamic.InFlightFences[m_CurrentFrame]);

			vkResetCommandBuffer(m_Vk.Dynamic.CmdBuffers.Draw[m_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
			recordDrawCommandBuffers(m_Vk.Dynamic.CmdBuffers.Draw[m_CurrentFrame], imageIndex);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = {m_Vk.Dynamic.ImageAvailableSemaphores[m_CurrentFrame]};
			VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_Vk.Dynamic.CmdBuffers.Draw[m_CurrentFrame];

			VkSemaphore signalSemaphores[] = {m_Vk.Dynamic.RenderFinishedSemaphores[m_CurrentFrame]};
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			VK_ASSERT (vkQueueSubmit(m_Vk.Dynamic.Queues.Graphics, 1, &submitInfo, m_Vk.Dynamic.InFlightFences[m_CurrentFrame]), "failed to submit draw command buffer!");

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = {m_Vk.Dynamic.Swapchain.Instance};
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &imageIndex;

			result = vkQueuePresentKHR(m_Vk.Dynamic.Queues.Presentation, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
				m_FramebufferResized = false;
				recreateSwapChain();
			} 
			else VK_ASSERT (result, "failed to present swap chain image!");
			

			m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}
		virtual void terminateVk () override {
			vkDeviceWaitIdle(m_Vk.Dynamic.LogicalDevice);

			cleanupSwapChainDependent();
			for (auto imageView : m_Vk.Dynamic.Swapchain.ImagesView) {
				vkDestroyImageView(m_Vk.Dynamic.LogicalDevice, imageView, nullptr);
			}
			vkDestroySwapchainKHR(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.Swapchain.Instance, nullptr);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vkDestroyBuffer(m_Vk.Dynamic.LogicalDevice, m_VkScene.UniformBuffers[i], nullptr);
				vkFreeMemory(m_Vk.Dynamic.LogicalDevice, m_VkScene.UniformBuffersMemory[i], nullptr);
			}

			vkDestroyDescriptorPool(m_Vk.Dynamic.LogicalDevice, m_Vk.Pipeline.DescriptorPool, nullptr);

			vkDestroyDescriptorSetLayout(m_Vk.Dynamic.LogicalDevice, m_Vk.Pipeline.Configs.DescriptorSetLayout, nullptr);

			vkDestroyBuffer(m_Vk.Dynamic.LogicalDevice, m_VkScene.IndexBuffer, nullptr);
			vkFreeMemory(m_Vk.Dynamic.LogicalDevice, m_VkScene.IndexBufferMemory, nullptr);

			vkDestroyBuffer(m_Vk.Dynamic.LogicalDevice, m_VkScene.VertexBuffer, nullptr);
			vkFreeMemory(m_Vk.Dynamic.LogicalDevice, m_VkScene.VertexBufferMemory, nullptr);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vkDestroySemaphore(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.RenderFinishedSemaphores[i], nullptr);
				vkDestroySemaphore(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.ImageAvailableSemaphores[i], nullptr);
				vkDestroyFence(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.InFlightFences[i], nullptr);
			}

			vkDestroyCommandPool(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.CmdPools.Graphics, nullptr);

			vkDestroyPipelineCache (m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.PipelineCache, nullptr);

			vkDestroyDevice(m_Vk.Dynamic.LogicalDevice, nullptr);

			if (m_Vk.EnableDebugging) {
				DestroyDebugUtilsMessengerEXT(m_Vk.Static.Instance, m_Vk.Static.DebugMessenger, nullptr);
			}

			vkDestroySurfaceKHR(m_Vk.Static.Instance, m_Vk.Static.Surface.Instance, nullptr);
			vkDestroyInstance(m_Vk.Static.Instance, nullptr);
		}
		virtual void cleanup () override {
			LOG_trace (std::source_location::current ().function_name ());
			delete m_Window;
		}
	private:
		Window* m_Window = nullptr;
		
		VulkanContext m_Vk;
		VulkanScene m_VkScene;

		bool m_Running = true;
		bool m_FramebufferResized = false;
		uint32_t m_CurrentFrame = 0;
	private:
		void createInstance() {
			auto& vk_instance = m_Vk.Static.Instance;
			auto& vk_extensions_to_enable  = m_Vk.Static.EnabledInstanceExtensions;
			const auto& req_instance_extns = m_Vk.MinimumRequiredInstanceExtensions;
			const auto& req_validation_layers = m_Vk.MinimumRequiredValidationLayers;
			const bool  enable_debugging = m_Vk.EnableDebugging;


			VkApplicationInfo app_info {
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Vulkan Example",
				.applicationVersion = VK_MAKE_VERSION (1, 0, 0),
				.pEngineName = "No Engine",
				.engineVersion = VK_MAKE_VERSION (1, 0, 0),
				.apiVersion = VK_API_VERSION_1_0,
			};
			// these 2 types are mostly required by every vk instance (name can vary across OS (s))
			vk_extensions_to_enable.clear ();
			for (const char* extn: req_instance_extns)
				vk_extensions_to_enable.insert (extn);
	
			if (enable_debugging)
				vk_extensions_to_enable.insert (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			std::vector <std::string> vk_extensions_supported; { // Vk Instance Extensions
				uint32_t extension_count = 0;
				vkEnumerateInstanceExtensionProperties (nullptr, &extension_count, nullptr);
				std::vector<VkExtensionProperties> extensions (extension_count);
				vkEnumerateInstanceExtensionProperties (nullptr, &extension_count, extensions.data ());
		
				vk_extensions_supported.reserve (extension_count);
				LOG_trace ("available extensions:");
				for (const auto& extension: extensions) {
					LOG_raw ("\n\t {}", extension.extensionName);
					vk_extensions_supported.push_back (extension.extensionName);
				}
				LOG_trace ("{} extensions supported", extension_count);
			}
			{ // Check validation layer support
				uint32_t layer_count;
				vkEnumerateInstanceLayerProperties (&layer_count, nullptr);
				std::vector<VkLayerProperties> validation_layers (layer_count);
				vkEnumerateInstanceLayerProperties (&layer_count, validation_layers.data ());

				std::vector<std::string_view> unavailable_layers;
				for (const auto layer_name : req_validation_layers) {
					bool layerFound = false;
		
					for (const auto& layer_properties : validation_layers) {
						if (layer_name != layer_properties.layerName) {
							layerFound = true;
							break;
						}
					}
					if (!layerFound) {
						unavailable_layers.push_back (layer_name);
					}
				}
				if (!unavailable_layers.empty ()) {
					LOG_error ("Missing Layer (s):");
					for (const auto extension_name: unavailable_layers)
						LOG_raw ("\n\t {}", extension_name);
					THROW_CORE_critical ("{:d} layer (s) not found", unavailable_layers.size ());
				}
			}
			{
				std::vector<char*> tmp (vk_extensions_to_enable.size ());
				for (int i = 0; auto extn: vk_extensions_to_enable) {
					tmp[i] = (char*) ((void*) (extn.data ())); ++i;
				}
				VkInstanceCreateInfo create_info {
					.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					// .pNext = (EnableValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info : nullptr), // to enable debugging while instance creation
					.pApplicationInfo = &app_info,
					.enabledLayerCount = uint32_t (enable_debugging ? std::size (req_validation_layers) : 0),
					.ppEnabledLayerNames = (enable_debugging ? &req_validation_layers[0] : nullptr),
					.enabledExtensionCount = uint32_t (std::size (tmp)),
					.ppEnabledExtensionNames = tmp.data (),
				};
				VK_ASSERT (vkCreateInstance (&create_info, nullptr, &vk_instance));
			}
		}
		void setupDebugging(const VkDebugUtilsMessageSeverityFlagsEXT message_severity_flags) {
			auto &debug_messenger = m_Vk.Static.DebugMessenger;
			const auto instance = m_Vk.Static.Instance;
			const auto message_type_flags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			VkBool32 (*VKAPI_PTR vk_debug_callback) (VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT*,void*) = [] 
				( VkDebugUtilsMessageSeverityFlagBitsEXT message_severity
				, VkDebugUtilsMessageTypeFlagsEXT message_type
				, const VkDebugUtilsMessengerCallbackDataEXT* callback_data
				, void* pUserData ) 
				{
					std::string msg_type;
					if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
						//if (!msg_type.empty ())
						//	msg_type += ", ";
						msg_type += "VkGeneral";
					} 
					if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
						if (!msg_type.empty ())
							msg_type += ", ";
						msg_type += "VkPerformance";
					} 
					if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
						if (!msg_type.empty ())
							msg_type += ", ";
						msg_type += "VkValidation";
					}
					if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
						LOG_error ("{:s}: {:s}", msg_type, callback_data->pMessage); __debugbreak ();
					} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
						LOG_warn ("{:s}: {:s}", msg_type, callback_data->pMessage);
					} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
						LOG_info ("{:s}: {:s}", msg_type, callback_data->pMessage);
					} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
						LOG_debug ("{:s}: {:s}", msg_type, callback_data->pMessage);
					} else {
						LOG_trace ("{:s}: {:s}", msg_type, callback_data->pMessage);
					}
		
					return VK_FALSE;
				};

			VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info { // Reusing later when creating debug messenger
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity = message_severity_flags,
				.messageType = message_type_flags,
				.pfnUserCallback = vk_debug_callback,
				.pUserData = nullptr // Optional
			};

			VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) 
				result = func (instance, &debug_messenger_info, nullptr, &debug_messenger);

			VK_ASSERT (result, "failed setting up vk_debug_messenger");
		}
		void createSurface() // surface of your window 
		{
			auto& surface = m_Vk.Static.Surface.Instance;
			auto& instance = m_Vk.Static.Instance;
			HWND  h_window = HWND(m_Window->GetNativeWindow ());

			#if NTKR_WINDOWS
				VkWin32SurfaceCreateInfoKHR surface_info {
					.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
					.hinstance = GetModuleHandle (nullptr),
					.hwnd = h_window,
				};
				VK_ASSERT (vkCreateWin32SurfaceKHR (instance, &surface_info, nullptr, &surface), "window surface creation failure");
			#else
				static_assert (false, "not implemented")
			#endif
		}
		void pickPhysicalDevice() {
			auto &selected_device = m_Vk.Static.PhysicalDevice.Handle;
			auto &selected_physical_mem_props = m_Vk.Static.PhysicalDevice.MemoryProperties;
			auto &selected_physical_properties = m_Vk.Static.PhysicalDevice.Properties;
			auto &selected_physical_features = m_Vk.Static.PhysicalDevice.Features;
			auto &selected_extensions = m_Vk.Static.PhysicalDevice.EnabledExtensions; 
			auto &selected_queue_family_indices = m_Vk.Static.QueueFamilyIndices;
			auto &selected_surface_formats = m_Vk.Static.Surface.Formats;
			auto &selected_surface_present_modes = m_Vk.Static.Surface.PresentModes;
			auto &selected_depth_format = m_Vk.Static.DepthFormat;
			auto &debug_marker_is_enableable = m_Vk.EnableDebugMarkers;
			const auto  instance = m_Vk.Static.Instance;
			const auto  surface = m_Vk.Static.Surface.Instance;
			const auto  req_extensions = m_Vk.MinimumRequiredDeviceExtensions;
			const auto  opt_extensions = m_Vk.OptionalDeviceExtensions;

			std::set<std::string_view> required_extensions_set, optional_extensions_set;
			for (const auto req_extn: req_extensions)
				required_extensions_set.insert (req_extn);
			for (const auto opt_extn: opt_extensions)
				optional_extensions_set.insert (opt_extn);
	
			struct {
				VkPhysicalDeviceProperties physical_properties;
				VkPhysicalDeviceFeatures physical_features;

				std::set<std::string_view> enableable_extns; // All required extensions present

				uint32_t graphics_family, presentation_family, compute_family, transfer_family;

				std::vector<VkSurfaceFormatKHR> surface_formats;
				std::vector<VkPresentModeKHR> surface_present_modes;

				inline void reset () {
					physical_properties = {}, physical_features = VkPhysicalDeviceFeatures{.samplerAnisotropy = VK_TRUE};
					enableable_extns.clear ();
					graphics_family = presentation_family = compute_family  = transfer_family = std::numeric_limits<uint32_t>::max ();
					surface_formats.clear ();
					surface_present_modes.clear ();
				}
			} cache;
			// cache.enableable_extns.reserve (std::size (req_extensions) + std::size (opt_extensions));
			auto test_device_suitable = [&] (VkPhysicalDevice device) -> bool { 
				cache.reset ();

				vkGetPhysicalDeviceProperties (device, &cache.physical_properties);
				vkGetPhysicalDeviceFeatures (device, &cache.physical_features);
		
				for (const auto req_extn: req_extensions)
					cache.enableable_extns.insert (req_extn);
				auto check_device_extensions_support = [&] () -> bool {
						uint32_t extension_count;
						vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, nullptr);
						std::vector<VkExtensionProperties> properties (extension_count);
						vkEnumerateDeviceExtensionProperties (device, nullptr, &extension_count, properties.data ());
				
						std::set<std::string_view> required_extensions = required_extensions_set, optional_extensions = optional_extensions_set;
			
						for (auto& extension: properties) {
							required_extensions.erase (extension.extensionName);

							auto old_size = optional_extensions.size ();
							optional_extensions.erase (extension.extensionName);
							if (optional_extensions.size () != old_size)
								cache.enableable_extns.insert (extension.extensionName); // optional_extensions get string_vews indirectly from m_Vk.OptionalDeviceExtensions

							if (required_extensions.empty () && optional_extensions.empty ()) break;
						}
						return required_extensions.empty ();
					};

				constexpr auto uint32_max = std::numeric_limits<uint32_t>::max ();
				auto get_queue_family_indices = [&] () -> bool {
						// Find required queue families
						uint32_t queue_families_count = 0;
						vkGetPhysicalDeviceQueueFamilyProperties (device, &queue_families_count, nullptr);
						std::vector<VkQueueFamilyProperties> queue_families (queue_families_count);
						vkGetPhysicalDeviceQueueFamilyProperties (device, &queue_families_count, queue_families.data ());

						bool dedicated_compute = false, dedicated_transfer = false;
						for (int i = 0; auto &queue_family: queue_families) {
							if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT && cache.graphics_family  == uint32_max) // std::numeric_limits<uint32_t>::max () + 1 == 0 & !0 = 1
								cache.graphics_family = i;
							if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT  && (cache.compute_family  == uint32_max || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && !dedicated_compute)
								cache.compute_family = i, dedicated_compute = (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;
							if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT && (cache.transfer_family == uint32_max || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && !dedicated_transfer)
								cache.transfer_family = i, dedicated_transfer = (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;

							if (cache.presentation_family == uint32_max) {
								VkBool32 presentation_support_extension = false;
								vkGetPhysicalDeviceSurfaceSupportKHR (device, i, surface, &presentation_support_extension);
								if (presentation_support_extension)
									cache.presentation_family = i;
							}
							++i;
						}
						return cache.graphics_family != uint32_max && cache.presentation_family != uint32_max && cache.compute_family != uint32_max && cache.transfer_family != uint32_max; // we are confirming only graphics and presentation as mandatory
					};

				auto query_swap_chain_support = [&] () -> bool {
						uint32_t format_count;
						vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &format_count, nullptr);
						if (format_count != 0) {
							cache.surface_formats.resize (format_count);
							vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &format_count, cache.surface_formats.data ());
						}
				
						uint32_t present_mode_count;
						vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &present_mode_count, nullptr);
						if (present_mode_count != 0) {
							cache.surface_present_modes.resize (present_mode_count);
							vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &present_mode_count, cache.surface_present_modes.data ());
						}

						return format_count != 0 && present_mode_count != 0;
					};

				if (check_device_extensions_support ())
					if (get_queue_family_indices ())
						if (query_swap_chain_support ()) {
							return true
								&& cache.physical_features.geometryShader
								&& cache.physical_features.samplerAnisotropy
							;
						}
		
				return false;
			};

			selected_device = VK_NULL_HANDLE;
			uint32_t device_count = 0;
			vkEnumeratePhysicalDevices (instance, &device_count, nullptr);
			if (device_count == 0)
				THROW_CORE_critical ("failed to find hardware with vulkan support");
			std::vector<VkPhysicalDevice> vk_enabled_devices (device_count);
			vkEnumeratePhysicalDevices (instance, &device_count, vk_enabled_devices.data ());

			for (auto device: vk_enabled_devices) {
				if (test_device_suitable (device)) {
					selected_device = device; 

					selected_physical_properties = cache.physical_properties;
					selected_physical_features = cache.physical_features;

					selected_extensions = std::move (cache.enableable_extns);

					selected_queue_family_indices.Graphics     = cache.graphics_family;
					selected_queue_family_indices.Presentation = cache.presentation_family;
					selected_queue_family_indices.Compute  = cache.compute_family;
					selected_queue_family_indices.Transfer = cache.transfer_family;

					selected_surface_formats = std::move (cache.surface_formats);
					selected_surface_present_modes = std::move (cache.surface_present_modes);

					if (selected_physical_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
						break; // iGPUs are good and all, but descrete ones are better
				}
			}
			if (selected_device == VK_NULL_HANDLE) 
				THROW_CORE_critical ("failed to find suitable hardware with required vulkan support");

			if (std::find (selected_extensions.begin (), selected_extensions.end (), std::string_view (VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) == selected_extensions.end ())
				debug_marker_is_enableable = false;

			vkGetPhysicalDeviceMemoryProperties (selected_device, &selected_physical_mem_props);
			// Since (m)any of depth formats can be optional, we need to find a suitable depth format to use
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
				vkGetPhysicalDeviceFormatProperties (selected_device, format, &format_props);
				// Format must support depth stencil attachment for optimal tiling
				if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
					selected_depth_format = format;
			}
		}
		void setDevicePrefrences () {
			// m_Vk.Dynamic.EnabledFeatures = m_Vk.Static.PhysicalFeatures; // All features

			// if (m_Vk.Static.Surface.Capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) 
			// 	m_Vk.Static.Surface.Capabilities.currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			m_Vk.Dynamic.EnabledFeatures = VkPhysicalDeviceFeatures{};
		}
		void createLogicalDevice(const VkQueueFlagBits queue_types) {
			auto& logical_device  = m_Vk.Dynamic.LogicalDevice;
			const auto  physical_device = m_Vk.Static.PhysicalDevice.Handle;
			const auto  queue_family_indices = m_Vk.Static.QueueFamilyIndices;
			const auto  enable_extensions = m_Vk.Static.PhysicalDevice.EnabledExtensions; // aka all extensions
			const auto  enable_features = m_Vk.Dynamic.EnabledFeatures;
			const auto  enable_validation_layers = m_Vk.MinimumRequiredValidationLayers;
			const bool  enable_debugging = m_Vk.EnableDebugging;

			std::set<uint32_t> unique_queue_families;

			if (queue_types & VK_QUEUE_GRAPHICS_BIT) {
				unique_queue_families.insert (queue_family_indices.Graphics); 
				unique_queue_families.insert (queue_family_indices.Presentation);
			}
			if (queue_types & VK_QUEUE_COMPUTE_BIT)
				unique_queue_families.insert (queue_family_indices.Compute);
			if (queue_types & VK_QUEUE_TRANSFER_BIT)
				unique_queue_families.insert (queue_family_indices.Transfer);

			std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
			queue_create_infos.reserve (unique_queue_families.size ());
			for (auto family_indice: unique_queue_families) {
				float queue_priority = 1.0f; // need more information on it
				queue_create_infos.push_back (VkDeviceQueueCreateInfo {
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = family_indice,
					.queueCount = 1,
					.pQueuePriorities = &queue_priority
				});
			}
			{
				std::vector<void*> device_extns;
				device_extns.reserve (std::size (enable_extensions));
				for (int i = 0; auto extn: enable_extensions) {
					device_extns.push_back ((void*)(extn.data ()));
					++i;
				}
				VkDeviceCreateInfo create_info {
					.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
					.queueCreateInfoCount = uint32_t (queue_create_infos.size ()),
					.pQueueCreateInfos = queue_create_infos.data (),
					.enabledLayerCount = uint32_t (enable_debugging ? std::size (enable_validation_layers) : 0),
					.ppEnabledLayerNames = (enable_debugging ? &enable_validation_layers[0] : nullptr),
					.enabledExtensionCount = uint32_t (device_extns.size ()),
					.ppEnabledExtensionNames = (const char* const*)(device_extns.data ()),
					.pEnabledFeatures = &enable_features,
				};
	
				VK_ASSERT (vkCreateDevice (physical_device, &create_info, nullptr, &logical_device), "Failed to create logical device");
			}
		}
		void getRequiredQueues (const VkQueueFlagBits queue_types) // can be (s) graphics, presentation, compute, transfer VkQueues
		{
			auto& logical_device = m_Vk.Dynamic.LogicalDevice;
			auto  queue_indices_copy = m_Vk.Static.QueueFamilyIndices;
			auto& queues  = m_Vk.Dynamic.Queues.as_arr;

			constexpr uint32_t uint32_max = std::numeric_limits<uint32_t>::max ();
			if (queue_types & VK_QUEUE_GRAPHICS_BIT == 0) {
				queue_indices_copy.Graphics = uint32_max; 
				queue_indices_copy.Presentation = uint32_max;
			}
			if (queue_types & VK_QUEUE_COMPUTE_BIT == 0)
				queue_indices_copy.Compute = uint32_max;
			if (queue_types & VK_QUEUE_TRANSFER_BIT == 0)
				queue_indices_copy.Transfer = uint32_max;

			for (int i = 0; uint32_t queue_family: queue_indices_copy.as_arr) {
				queues[i] = VK_NULL_HANDLE; // reset non-required queue
				if (queue_family != uint32_max)
					vkGetDeviceQueue (logical_device, queue_family, 0, &queues[i]);
				++i;
			};
		} 

		void prepare () {
			if (m_Vk.EnableDebugMarkers)
				NutCracker::DebugMarker::setup (m_Vk.Dynamic.LogicalDevice);

			
			const bool add_depth = false;

			createCommandPool (VK_QUEUE_GRAPHICS_BIT);
			createCommandBuffers();
			selectSwapSurfaceOptions ();
			setupSwapChain ();
			createSynchronizationPrimitives ();
			if (add_depth)
				setupDepthStencil ();

			setupRenderPass(add_depth);
			setupFramebuffers();
			createPipelineCache ();

			// Pipeline related
			prepareDataBuffers    ();
			prepareUniformBuffers ();
			createDescriptorSetLayout ();
			std::array <VkDynamicState, 2> dynamic_states { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			preparePipeline (dynamic_states);
			prepareDescriptorPool ();
			prepareDescriptorSets ();
		}
		void createCommandPool (const VkQueueFlagBits pool_for_queue_types) {
			auto &graphics_cmd_pool = m_Vk.Dynamic.CmdPools.Graphics;
			const auto device = m_Vk.Dynamic.LogicalDevice;
			const auto graphics_family = m_Vk.Static.QueueFamilyIndices.Graphics;
			const auto compute_family = m_Vk.Static.QueueFamilyIndices.Compute;
			const auto transfer_family = m_Vk.Static.QueueFamilyIndices.Transfer;

			VkCommandPoolCreateInfo pool_info {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			};
			constexpr auto uint32_max = std::numeric_limits<uint32_t>::max ();
			//std::vector<uint32_t> pool_of_queue_types;
			if (pool_for_queue_types & VK_QUEUE_GRAPHICS_BIT && uint32_max != graphics_family) {
				pool_info.queueFamilyIndex = graphics_family; // reusing with changes
				VK_ASSERT (vkCreateCommandPool (device, &pool_info, nullptr, &graphics_cmd_pool), "failed to create command pool!");
			}
			if (pool_for_queue_types & VK_QUEUE_COMPUTE_BIT  && uint32_max != compute_family) {
				pool_info.queueFamilyIndex = compute_family;
				THROW_critical ("not implemented"); // pool_of_queue_types.push_back (compute_family);
			}
			if (pool_for_queue_types & VK_QUEUE_TRANSFER_BIT && uint32_max != transfer_family) {
				pool_info.queueFamilyIndex = transfer_family;
				THROW_critical ("not implemented"); // pool_of_queue_types.push_back (transfer_family);
			}

			//if (pool_of_queue_types.empty ())
			//	THROW_critical ("no pool to create");
		}
		void createCommandBuffers() {
			auto& cmd_buffs = m_Vk.Dynamic.CmdBuffers;
			const auto cmd_pools = m_Vk.Dynamic.CmdPools;
			const auto device = m_Vk.Dynamic.LogicalDevice;

			// Create command buffer // Currently Only for graphics
			//std::vector<uint32_t> pool_of_queue_types;
			if (cmd_pools.Graphics != VK_NULL_HANDLE) {
				Hlpr::AllocateCommandBuffers (cmd_buffs.Draw, device, cmd_pools.Graphics);
			}
			if (cmd_pools.Compute != VK_NULL_HANDLE) {
				Hlpr::AllocateCommandBuffers (std::span{&cmd_buffs.Compute, 1}, device, cmd_pools.Compute);
			}
			if (cmd_pools.Transfer != VK_NULL_HANDLE) {
				Hlpr::AllocateCommandBuffers (std::span{&cmd_buffs.Transfer, 1}, device, cmd_pools.Transfer);
			}
		}
		void selectSwapSurfaceOptions () // image format and color space
		{
			auto& selected_image_format = m_Vk.Dynamic.Swapchain.ImageFormat;
			auto& selected_color_space  = m_Vk.Dynamic.Swapchain.ColorSpace ; 
			auto& present_mode_selected = m_Vk.Dynamic.Swapchain.PresentMode;
			const auto& surface_formats = m_Vk.Static.Surface.Formats;
			const auto& surface_present_mode_supported = m_Vk.Static.Surface.PresentModes;
			// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
			// there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
			if ((std::size (surface_formats) == 1) && (surface_formats.begin ()->format == VK_FORMAT_UNDEFINED))
			{
				selected_image_format = VK_FORMAT_B8G8R8A8_SRGB;
				selected_color_space  = surface_formats.begin ()->colorSpace;
			}
			else
			{
				// iterate over the list of available surface format and
				// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
				bool found_B8G8R8A8_SRGB = false;
				for (const auto& surface_format : surface_formats)
				{
					if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						selected_image_format = surface_format.format;
						selected_color_space  = surface_format.colorSpace;
						found_B8G8R8A8_SRGB   = true;
						break;
					}
				}

				// in case VK_FORMAT_B8G8R8A8_UNORM is not available
				// select the first available color format
				if (!found_B8G8R8A8_SRGB) {
					selected_image_format = surface_formats.begin ()->format,
					selected_color_space  = surface_formats.begin ()->colorSpace; }
			}

			// choose swap presentation mode
			present_mode_selected = VK_PRESENT_MODE_FIFO_KHR; // default
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
		}
		void createSynchronizationPrimitives () {
			auto& image_available_semaphores  = m_Vk.Dynamic.ImageAvailableSemaphores;
			auto& render_available_semaphores = m_Vk.Dynamic.RenderFinishedSemaphores;
			auto& in_flight_fences = m_Vk.Dynamic.InFlightFences;
			const auto  device     = m_Vk.Dynamic.LogicalDevice;
			const auto& cmd_buffs  = m_Vk.Dynamic.CmdBuffers.Draw;
			{
				// Create syncronisation objects
				VkSemaphoreCreateInfo semaphore_info { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

				for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
					VK_ASSERT (vkCreateSemaphore (device, &semaphore_info, nullptr, &image_available_semaphores[i]),  "failed to create semaphore = ImageAvailableSemaphores[{:d}]", i);
					VK_ASSERT (vkCreateSemaphore (device, &semaphore_info, nullptr, &render_available_semaphores[i]), "failed to create semaphore = RenderFinishedSemaphores[{:d}]", i);
				}
			} {
				// Wait fences to sync command buffer access
				VkFenceCreateInfo fence_info {
					.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
					.flags = VK_FENCE_CREATE_SIGNALED_BIT
				};

				static_assert ((sizeof(in_flight_fences) / sizeof(in_flight_fences[0])) == (sizeof(cmd_buffs) / sizeof(cmd_buffs[0]))); 
				// against std::size, i know this looks ugly refer 
				// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2280r1.html

				for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) { 
					VK_ASSERT (vkCreateFence (device, &fence_info, nullptr, &in_flight_fences[i]), "failed to create fence = InFlightFence[{:d}]", i);
				}
			}
		}
		void setupSwapChain () {
			auto  old_swapchain = std::move (m_Vk.Dynamic.Swapchain); // saving the old
			auto &new_swapchain = m_Vk.Dynamic.Swapchain;
			const auto [fb_width, fb_height] = m_Window->GetFramebufferSize ();
			const auto  surface = m_Vk.Static.Surface.Instance;
			const auto  surface_capabilities_supported = m_Vk.Static.Surface.Capabilities (m_Vk.Static.PhysicalDevice.Handle);
			const auto  graphics_family_indice = m_Vk.Static.QueueFamilyIndices.Graphics;
			const auto  presentation_family_indice = m_Vk.Static.QueueFamilyIndices.Presentation;
			const auto  logical_device = m_Vk.Dynamic.LogicalDevice;
			const auto  old_swapchin_colorspace = old_swapchain.ColorSpace;
			const auto  old_swapchin_format = old_swapchain.ImageFormat;
			const auto  present_mode_selected = old_swapchain.PresentMode;

			// set swapchain swap extent
			VkExtent2D extent_selected;
			if (surface_capabilities_supported.currentExtent.width != std::numeric_limits<uint32_t>::max ()) {
				extent_selected = surface_capabilities_supported.currentExtent;
			} else {
				extent_selected = VkExtent2D {
					.width  = std::clamp (fb_width , surface_capabilities_supported.minImageExtent.width , surface_capabilities_supported.maxImageExtent.width),
					.height = std::clamp (fb_height, surface_capabilities_supported.minImageExtent.height, surface_capabilities_supported.maxImageExtent.height)
				};
			}

			// Find a supported composite alpha format (not all devices support alpha opaque)
			VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
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

			uint32_t image_count = std::min (surface_capabilities_supported.minImageCount +  1
											,surface_capabilities_supported.maxImageCount != 0 ?
												surface_capabilities_supported.maxImageCount :
												std::numeric_limits<uint32_t>::max ());
			VkSwapchainCreateInfoKHR swapchain_create_info {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = surface,

				.minImageCount = image_count,
				.imageFormat = old_swapchin_format,
				.imageColorSpace = old_swapchin_colorspace,
				.imageExtent = extent_selected,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

				.preTransform = surface_capabilities_supported.currentTransform,
				.compositeAlpha = composite_alpha,
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
				swapchain_create_info.queueFamilyIndexCount = uint32_t (std::size (queue_family_indices));
				swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
			}

			new_swapchain.ImageExtent = extent_selected;
			new_swapchain.ImageFormat = swapchain_create_info.imageFormat;
			new_swapchain.ColorSpace  = swapchain_create_info.imageColorSpace;
			VK_ASSERT (vkCreateSwapchainKHR (logical_device, &swapchain_create_info, nullptr, &new_swapchain.Instance), "failed to create swapchain !");

			// cleanup old swapchain
			if (old_swapchain.Instance != VK_NULL_HANDLE) {
				for (auto &image_view: old_swapchain.ImagesView) 
					vkDestroyImageView (logical_device, image_view, nullptr);
				vkDestroySwapchainKHR (logical_device, old_swapchain.Instance, nullptr);
			}

			vkGetSwapchainImagesKHR (logical_device, new_swapchain.Instance, &image_count, nullptr);
			new_swapchain.ImagesView = std::move (old_swapchain.ImagesView);
			new_swapchain.ImagesView.resize (image_count);
			new_swapchain.Images = std::move (old_swapchain.Images);
			new_swapchain.Images.resize (image_count);
			new_swapchain.ImageCount = image_count;

			vkGetSwapchainImagesKHR (logical_device, new_swapchain.Instance, &image_count, new_swapchain.Images.data ());

			for (size_t i = 0; i < image_count; ++i) {
				VkImageViewCreateInfo create_info {
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
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

				VK_ASSERT (vkCreateImageView (logical_device, &create_info, nullptr, &new_swapchain.ImagesView[i]), "failed to create image_view = swapchain.ImageViews[{:d}]!", i);
			}
		}
		void setupDepthStencil () {
			auto &depth_stencil = m_Vk.Dynamic.DepthStencil;
			const auto device   = m_Vk.Dynamic.LogicalDevice;
			const auto phy_mem_props = m_Vk.Static.PhysicalDevice.MemoryProperties;
			const auto depth_format  = m_Vk.Static.DepthFormat;
			const auto image_extent  = m_Vk.Dynamic.Swapchain.ImageExtent;
			if (depth_format == VK_FORMAT_UNDEFINED) {
				LOG_error (__FUNCTION__ ": depth format is undefined");
				return;
			}

			VkImageCreateInfo image_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = depth_format,
				.extent = { image_extent.width, image_extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			};
	
			VK_ASSERT (vkCreateImage (device, &image_info, nullptr, &depth_stencil.Image), "unable to create depth image");
			VkMemoryRequirements mem_reqs{};
			vkGetImageMemoryRequirements (device, depth_stencil.Image, &mem_reqs);

			VkMemoryAllocateInfo mem_alloc_info {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = mem_reqs.size,
				.memoryTypeIndex = Hlpr::FindVkMemoryType (phy_mem_props, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
			};
			VK_ASSERT (vkAllocateMemory (device, &mem_alloc_info, nullptr, &depth_stencil.ImageMem), "cannot allocate image memory to depth stencil");
			VK_ASSERT (vkBindImageMemory (device, depth_stencil.Image, depth_stencil.ImageMem, 0), "cannot bind image memory to depth stencil image");

			VkImageViewCreateInfo image_view_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = depth_stencil.Image,
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
	
			VK_ASSERT (vkCreateImageView (device, &image_view_info, nullptr, &depth_stencil.ImageView), "failed to create depth image");
		}
		void setupRenderPass (bool add_depth_stencil_to_attachment = false) {
			auto& render_pass = m_Vk.Dynamic.RenderPass;
			const auto  device = m_Vk.Dynamic.LogicalDevice;
			const auto& swapchain = m_Vk.Dynamic.Swapchain;
			const auto  depth_format = m_Vk.Static.DepthFormat;

			const bool  add_depth = depth_format != VK_FORMAT_UNDEFINED ? add_depth_stencil_to_attachment:false;
			if (depth_format == VK_FORMAT_UNDEFINED)
				LOG_error (__FUNCTION__ ": depth format is undefined");
			

			std::vector<VkAttachmentDescription> attachments; // for color pass and depth pass
			std::vector<VkAttachmentReference> color_attachments_ref;
			VkAttachmentReference depth_attachment_ref;

			// 1 subpass
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

			constexpr int subpass_count = 1;

			// add all color attachents
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
				color_attachments_ref.push_back (VkAttachmentReference {
					.attachment = counter,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				});
			} // ... likewise all desired color attachmens
	
			// attach all color attachments to concerning subpass
			subpass_description.colorAttachmentCount = color_attachments_ref.size ();
			subpass_description.pColorAttachments = color_attachments_ref.data ();

			// add depth attachment
			if (add_depth) {
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
	
				subpass_description.pDepthStencilAttachment = &depth_attachment_ref;
			}

			// subpass dependencies = total_subpass_in_the_render_pass + 1
			// THIS IS HOW I UNDERSTAND
			// 0th one brings attachments from bottom of the pipeline to top
			// subsequent one's are passed through subpasses
			// last one brings attachments from last subpass to bottom of the pipeline
			// .------\/ bottom to 1st
			// |    1st subpasss
			// |	  \/ 1st to 2nd
			// |	 ...
			// |	  \/
			// |	n-th subpass
			// `------\/ n-th to bottom

			decltype (subpass_description) subpasses [subpass_count] = {subpass_description};
			std::array<VkSubpassDependency, subpass_count + 1> subpass_dependencies;
			VkSubpassDependency& first_subpass_dependecy = subpass_dependencies.front ();
			VkSubpassDependency& last_subpass_dependecy = subpass_dependencies.back ();
		
			first_subpass_dependecy = VkSubpassDependency {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			};

			// for (int i = 1; i < subpass_count; i++) { 
			//	subpass_depn = VkSubpassDependency {
			//		.srcSubpass = i - 1,
			//		.dstSubpass = i,
			//	...fill...};  }
		
			last_subpass_dependecy = VkSubpassDependency {
				.srcSubpass = subpass_count - 1, // subpass_index ∈ [0 N), N = subpass_count
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			};

			VkRenderPassCreateInfo render_pass_info {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = uint32_t (std::size (attachments)), // all attachments involved
				.pAttachments = &attachments[0],
				.subpassCount = uint32_t (std::size (subpasses)), // all subpasses
				.pSubpasses = &subpasses[0],
				.dependencyCount = std::size (subpass_dependencies), // all subpass pependency
				.pDependencies = &subpass_dependencies[0],
			};

			VK_ASSERT (vkCreateRenderPass (device, &render_pass_info, nullptr, &render_pass), "failed to create render pass");
		}
		void setupFramebuffers (bool add_depth_attachment_to_framebuffer = false) {
			constexpr uint32_t num_color_attachments = 1; // shader outputs

			auto& framebuffers = m_Vk.Dynamic.Framebuffers;
			const auto  device = m_Vk.Dynamic.LogicalDevice;
			const auto  render_pass  = m_Vk.Dynamic.RenderPass;
			const auto  image_extent = m_Vk.Dynamic.Swapchain.ImageExtent;
			const auto  depth_image_view = m_Vk.Dynamic.DepthStencil.ImageView;
			const auto  swapchain_images_count = m_Vk.Dynamic.Swapchain.ImageCount;
			const auto& color_attachment_0_imageviews = m_Vk.Dynamic.Swapchain.ImagesView;
			const auto  depth_format = m_Vk.Static.DepthFormat;

			// Every color attachment of every frame will have an image
			const std::vector<VkImageView>* color_image_views [num_color_attachments] = { &color_attachment_0_imageviews /*, ... */ };

			const bool  add_depth = depth_format != VK_FORMAT_UNDEFINED ? add_depth_attachment_to_framebuffer:false;
			if (depth_format == VK_FORMAT_UNDEFINED)
				LOG_error (__FUNCTION__ ": depth format is undefined");

			VkImageView attachments [num_color_attachments + 1]; // +1 of depth
			const uint32_t num_of_attachments = num_color_attachments + (add_depth ? 1 : 0);

			VkFramebufferCreateInfo framebuffer_info = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = render_pass,
				.attachmentCount = num_of_attachments,
				.pAttachments = &attachments[0],
				.width = image_extent.width,
				.height = image_extent.height,
				.layers = 1,
			};

			// last one is depth
			attachments [num_color_attachments] = depth_image_view;

			// for every swap (out from renderpass) in swapchain create a framebuffer, here it is
			const int total_framebuffers = swapchain_images_count;
			framebuffers.resize (total_framebuffers);
			
			for (int counter = 0; counter < total_framebuffers; counter++) {
				for (int i = 0; i < num_color_attachments; i++) {
					attachments[i] = color_image_views[i]->at (counter);
				}

				VK_ASSERT (vkCreateFramebuffer (device, &framebuffer_info, nullptr, &framebuffers[counter]), "failed to create framebuffers");
			}
		}
		void createPipelineCache () {
			auto& pipeline_cache = m_Vk.Dynamic.PipelineCache;
			const auto  device = m_Vk.Dynamic.LogicalDevice;

			VkPipelineCacheCreateInfo pipeline_cache_create_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

			VK_ASSERT (vkCreatePipelineCache (device, &pipeline_cache_create_info, nullptr, &pipeline_cache), "failed to create pipeline cache");
		}
		void prepareDataBuffers () {
			auto& scene = m_VkScene;
			const auto logical_device  = m_Vk.Dynamic.LogicalDevice;
			const auto physical_device = m_Vk.Static.PhysicalDevice.Handle;
			const auto command_pool = m_Vk.Dynamic.CmdPools.Graphics;
			const auto transfer_queue = m_Vk.Dynamic.Queues.Graphics;
			const auto device_properties = m_Vk.Static.PhysicalDevice.Properties;
			const auto device_mem_properties = m_Vk.Static.PhysicalDevice.MemoryProperties;
			const auto device_features = m_Vk.Static.PhysicalDevice.Features;

			std::array<VkCommandBuffer, 1> data_copy_cmd_buff;
			Hlpr::AllocateCommandBuffers (data_copy_cmd_buff, logical_device, command_pool);

			// Quad Vertex Data
			{
				VkDeviceSize buffer_size = sizeof (s_QuadVertices);
				const void *data_buff = s_QuadVertices.data ();
		
				struct {
					VkBuffer Buffer;
					VkDeviceMemory BufferMemory;
				} staging;

				Hlpr::CreateVkBuffer (staging.Buffer, staging.BufferMemory
					,logical_device, physical_device, buffer_size
					,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, device_mem_properties
					,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data_buff);
		
				Hlpr::CreateVkBuffer (scene.VertexBuffer, scene.VertexBufferMemory
					,logical_device, physical_device, buffer_size
					,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, device_mem_properties
					,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				Hlpr::BeginSingleTimeVkCommands (data_copy_cmd_buff[0]);
				VkBufferCopy copy_region{
					.srcOffset = 0, // Optional
					.dstOffset = 0, // Optional
					.size = buffer_size
				};
				vkCmdCopyBuffer (data_copy_cmd_buff[0], staging.Buffer, scene.VertexBuffer, 1, &copy_region);
				Hlpr::EndSingleVkCommand (transfer_queue, data_copy_cmd_buff[0]);

				vkDestroyBuffer (logical_device, staging.Buffer, nullptr);
				vkFreeMemory (logical_device, staging.BufferMemory, nullptr);
			}
			// Quad Index Data
			{
				VkDeviceSize buffer_size = sizeof (s_QuadIndices);
				const void* data_buff = s_QuadIndices.data ();
		
				struct {
					VkBuffer Buffer;
					VkDeviceMemory BufferMemory;
				} staging;

				Hlpr::CreateVkBuffer (staging.Buffer, staging.BufferMemory
					,logical_device, physical_device, buffer_size
					,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, device_mem_properties
					,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data_buff);
		
				Hlpr::CreateVkBuffer (scene.IndexBuffer, scene.IndexBufferMemory
					,logical_device, physical_device, buffer_size
					,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, device_mem_properties
					,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				Hlpr::BeginSingleTimeVkCommands (data_copy_cmd_buff[0]);
				VkBufferCopy copy_region{
					.srcOffset = 0, // Optional
					.dstOffset = 0, // Optional
					.size = buffer_size
				};
				vkCmdCopyBuffer (data_copy_cmd_buff[0], staging.Buffer, scene.IndexBuffer, 1, &copy_region);
				Hlpr::EndSingleVkCommand (transfer_queue, data_copy_cmd_buff[0]);

				vkDestroyBuffer (logical_device, staging.Buffer, nullptr);
				vkFreeMemory (logical_device, staging.BufferMemory, nullptr);
			}
		}
		void prepareUniformBuffers () {
			auto& scene = m_VkScene;
			const auto logical_device = m_Vk.Dynamic.LogicalDevice;
			const auto physical_device = m_Vk.Static.PhysicalDevice.Handle;
			const auto device_mem_properties = m_Vk.Static.PhysicalDevice.MemoryProperties;
			const auto command_pool = m_Vk.Dynamic.CmdPools.Graphics;
			const auto transfer_queue = m_Vk.Dynamic.Queues.Graphics;

			VkDeviceSize buffer_size = sizeof(UniformBufferObject); // sizeof (s_SceneUniform);
			{ // uniform buffers

				for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
					Hlpr::CreateVkBuffer (m_VkScene.UniformBuffers[i], m_VkScene.UniformBuffersMemory[i]
						,logical_device, physical_device, buffer_size
						,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, device_mem_properties
						,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				}
			}
		}
		void createDescriptorSetLayout () {
			auto &descriptor_set_layout = m_Vk.Pipeline.Configs.DescriptorSetLayout;
			auto &pipeline_layout = m_Vk.Pipeline.Layout;
			const auto  device = m_Vk.Dynamic.LogicalDevice;

			VkDescriptorSetLayoutBinding bindings[] = {
			// vertex shader
				VkDescriptorSetLayoutBinding { // for ProjectionView
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
					.pImmutableSamplers = nullptr,
				}
			};

			VkDescriptorSetLayoutCreateInfo layout_info {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = uint32_t (std::size (bindings)),
				.pBindings = bindings,
			};

			VK_ASSERT (vkCreateDescriptorSetLayout (device, &layout_info, nullptr, &descriptor_set_layout), "failed to create descriptor set layout!");

			// Pipeline layouts contain the information about shader inputs of a given pipeline. 
			// It’s here where we would configure our push-constants and descriptor sets
			VkPipelineLayoutCreateInfo pipeline_layout_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = 1,
				.pSetLayouts = &descriptor_set_layout
			};
			
			VK_ASSERT (vkCreatePipelineLayout (device, &pipeline_layout_info, nullptr, &pipeline_layout), "failed to create pipeline layout");
		}
		void prepareDescriptorPool () {
			auto &descriptor_pool = m_Vk.Pipeline.DescriptorPool;
			const auto  device = m_Vk.Dynamic.LogicalDevice;

			// Create discription pool
			VkDescriptorPoolSize pool_sizes[] = { // total pool size
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = uint32_t (MAX_FRAMES_IN_FLIGHT)},
			};
			VkDescriptorPoolCreateInfo pool_info {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = MAX_FRAMES_IN_FLIGHT,
				.poolSizeCount = uint32_t (std::size (pool_sizes)),
				.pPoolSizes = pool_sizes,
			};

			VK_ASSERT (vkCreateDescriptorPool (device, &pool_info, nullptr, &descriptor_pool), "failed to create descriptor pool");
		}
		void prepareDescriptorSets ()  {
			auto &descriptor_sets = m_Vk.Pipeline.Configs.DescriptorSets;
			auto &scene = m_VkScene;
			const auto  descriptor_set_layout = m_Vk.Pipeline.Configs.DescriptorSetLayout;
			const auto  descriptor_pool = m_Vk.Pipeline.DescriptorPool;
			const auto  device = m_Vk.Dynamic.LogicalDevice;
			const auto  sizeof_UniformSceneObject = sizeof (UniformBufferObject); // sizeof (s_SceneUniform);

			// Create discription sets
			VkDescriptorSetLayout set_layouts[MAX_FRAMES_IN_FLIGHT];
			for (auto& set_layout: set_layouts) {
				set_layout = descriptor_set_layout;
			}
	
			VkDescriptorSetAllocateInfo alloc_info {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = descriptor_pool,
				.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
				.pSetLayouts = set_layouts
			};

			VK_ASSERT (vkAllocateDescriptorSets (device, &alloc_info, descriptor_sets), "failed to allocate descriptor sets!");

			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
				VkDescriptorBufferInfo buffer_info {
					.buffer = scene.UniformBuffers[i],
					.offset = 0,
					.range = sizeof_UniformSceneObject
				};

				VkWriteDescriptorSet descriptor_writes[] = {{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = descriptor_sets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &buffer_info,
				}};

				vkUpdateDescriptorSets (device, uint32_t (std::size (descriptor_writes)), descriptor_writes, 0, nullptr);
			}
		}
//// TODO
		void preparePipeline (const std::span<VkDynamicState> dynamic_states_to_enable, const bool add_depth_stencil_to_attachment = false) {
			auto& pipeline = m_Vk.Pipeline;
			const auto  device = m_Vk.Dynamic.LogicalDevice;
			const auto  render_pass = m_Vk.Dynamic.RenderPass;
			const auto  pipeline_cache = m_Vk.Dynamic.PipelineCache;
			const bool  add_depth = m_Vk.Static.DepthFormat != VK_FORMAT_UNDEFINED ? add_depth_stencil_to_attachment:false;

			// prepare data
			auto vert_shader_module = Hlpr::CreateVkShaderModule (device, 
												NutCracker::Assets::Files::ReadFromDisk (PROJECT_ROOT_LOCATION "assets/shaders/test/test-vert.sprv"));
			auto frag_shader_module = Hlpr::CreateVkShaderModule (device,
												NutCracker::Assets::Files::ReadFromDisk (PROJECT_ROOT_LOCATION "assets/shaders/test/test-frag.sprv"));

			// Variable
			VkPipelineShaderStageCreateInfo shader_stages_info[] = {
				VkPipelineShaderStageCreateInfo { // vert_shader_stage_info
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = vert_shader_module,
					.pName = "main"
				},
				VkPipelineShaderStageCreateInfo { // frag_shader_stage_info
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = frag_shader_module,
					.pName = "main"
				},
			};
			VkVertexInputBindingDescription vertex_binding_descriptions [] = {Vertex::getBindingDescription ()};
			VkVertexInputAttributeDescription vertex_attrib_descriptions[ Vertex::getAttributeDescriptions ().size () ]; {
				auto vert_attrib_dsr = Vertex::getAttributeDescriptions<0> ();
				int i = 0;
				for (auto& dscriptn: vert_attrib_dsr) {
					vertex_attrib_descriptions[i] = dscriptn; 
					i++;
				}
			}
			VkPipelineVertexInputStateCreateInfo vertex_input_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = uint32_t (std::size (vertex_binding_descriptions)),
				.pVertexBindingDescriptions = vertex_binding_descriptions, // Optional
				.vertexAttributeDescriptionCount = uint32_t (std::size (vertex_attrib_descriptions)),
				.pVertexAttributeDescriptions = vertex_attrib_descriptions,
			};
			VkViewport viewport{
				.x = 0.0f,
				.y = 0.0f,
				.width = (float) m_Vk.Dynamic.Swapchain.ImageExtent.width,
				.height = (float) m_Vk.Dynamic.Swapchain.ImageExtent.height,
				.minDepth = 0.0f,
				.maxDepth = 1.0f,
			};
			VkRect2D scissor{
				.offset = {0, 0},
				.extent = m_Vk.Dynamic.Swapchain.ImageExtent,
			};
			VkPipelineViewportStateCreateInfo viewport_state_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = 1,
				//.pViewports = &viewport, // will be set when recording buffer if dynamic state enabled
				.scissorCount = 1,
				//.pScissors = &scissor, // will be set when recording buffer
			};
			// dynamic_states_to_enable = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			// Add to pipelineCreateInfo if (dynamic_states_to_enable != empty)
			VkPipelineDynamicStateCreateInfo dynamic_state_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.flags = 0,
				.dynamicStateCount = uint32_t (dynamic_states_to_enable.size ()),
				.pDynamicStates = dynamic_states_to_enable.data (),
			};

			// constants
			VkPipelineInputAssemblyStateCreateInfo input_assembly_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.primitiveRestartEnable = VK_FALSE
			};
			VkPipelineRasterizationStateCreateInfo rasterizer_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_BACK_BIT,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE,

				.lineWidth = 1.0f
			};
			VkPipelineColorBlendAttachmentState color_blend_attachment {
				.blendEnable = VK_TRUE,

				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
				.colorBlendOp = VK_BLEND_OP_ADD, // Optional
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
				.alphaBlendOp = VK_BLEND_OP_ADD, // Optional

				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			};
			VkPipelineColorBlendStateCreateInfo color_blending_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.logicOpEnable = VK_FALSE,
				.logicOp = VK_LOGIC_OP_COPY, // Optional
				.attachmentCount = 1,
				.pAttachments = &color_blend_attachment,
				.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}, // Optional
			};
			VkPipelineMultisampleStateCreateInfo multisampling_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.flags = 0, 
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				.sampleShadingEnable = VK_FALSE,
				.minSampleShading = 1.0f, 
			};
			// Add to pipelineCreateInfo if (add_depth)
			VkPipelineDepthStencilStateCreateInfo depth_stencil_info {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable =  VK_TRUE,  
				.depthWriteEnable = VK_TRUE,
				.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
				.back = {.compareOp = VK_COMPARE_OP_ALWAYS},
			};

			VkGraphicsPipelineCreateInfo pipelineInfo{
				.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				.stageCount = uint32_t (std::size (shader_stages_info)),
				.pStages = shader_stages_info,
				.pVertexInputState = &vertex_input_info,
				.pInputAssemblyState = &input_assembly_info,
				.pViewportState = &viewport_state_info,
				.pRasterizationState = &rasterizer_info,
				.pMultisampleState = &multisampling_info, 
				.pDepthStencilState = add_depth ? &depth_stencil_info:nullptr,
				.pColorBlendState = &color_blending_info,
				.pDynamicState = dynamic_states_to_enable.empty () ? nullptr : &dynamic_state_info,
				.layout = pipeline.Layout,
				.renderPass = render_pass,
				.subpass = 0,
				.basePipelineHandle = VK_NULL_HANDLE,
			};

			VK_ASSERT (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.Instance), "failed to create graphics pipeline!");

			vkDestroyShaderModule(m_Vk.Dynamic.LogicalDevice, frag_shader_module, nullptr);
			vkDestroyShaderModule(m_Vk.Dynamic.LogicalDevice, vert_shader_module, nullptr);
		}

//#######################################################################################
//#######################################################################################
//#######################################################################################
//#######################################################################################

		void cleanupSwapChainDependent() {
			for (auto framebuffer : m_Vk.Dynamic.Framebuffers) {
				vkDestroyFramebuffer(m_Vk.Dynamic.LogicalDevice, framebuffer, nullptr);
			}

			vkDestroyPipeline(m_Vk.Dynamic.LogicalDevice, m_Vk.Pipeline.Instance, nullptr);
			vkDestroyPipelineLayout(m_Vk.Dynamic.LogicalDevice, m_Vk.Pipeline.Layout, nullptr);
			vkDestroyRenderPass(m_Vk.Dynamic.LogicalDevice, m_Vk.Dynamic.RenderPass, nullptr);
		}
		void recreateSwapChain() {
			auto [width, height] = m_Window->GetFramebufferSize ();
			while (width == 0 || height == 0) {
				std::this_thread::sleep_for (std::chrono::milliseconds (20));
				m_Window->PollForEvents ();
				auto [_width, _height] = m_Window->GetFramebufferSize ();
				width = _width, height = _height;
			}

			vkDeviceWaitIdle (m_Vk.Dynamic.LogicalDevice);

			cleanupSwapChainDependent();

			setupSwapChain  ();
			setupRenderPass ();
			std::array <VkDynamicState, 2> dynamic_states { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			preparePipeline (dynamic_states);
			setupFramebuffers ();
		}

		void recordDrawCommandBuffers (VkCommandBuffer commandBuffer, uint32_t imageIndex) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VK_ASSERT (vkBeginCommandBuffer(commandBuffer, &beginInfo), "failed to begin recording command buffer!");

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_Vk.Dynamic.RenderPass;
			renderPassInfo.framebuffer = m_Vk.Dynamic.Framebuffers[imageIndex];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent = m_Vk.Dynamic.Swapchain.ImageExtent;

			VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Vk.Pipeline.Instance);

				VkBuffer vertexBuffers[] = {m_VkScene.VertexBuffer};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(commandBuffer, m_VkScene.IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Vk.Pipeline.Layout, 0, 1, &m_Vk.Pipeline.Configs.DescriptorSets[m_CurrentFrame], 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(s_QuadIndices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffer);

			VK_ASSERT (vkEndCommandBuffer(commandBuffer), "failed to record command buffer!");
		}
		void  updateUniformBuffer (uint32_t currentImage) {
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			UniformBufferObject ubo{};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(45.0f), m_Vk.Dynamic.Swapchain.ImageExtent.width / (float) m_Vk.Dynamic.Swapchain.ImageExtent.height, 0.1f, 10.0f);
			ubo.proj[1][1] *= -1;

			void* data;
			vkMapMemory(m_Vk.Dynamic.LogicalDevice, m_VkScene.UniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
				memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(m_Vk.Dynamic.LogicalDevice, m_VkScene.UniformBuffersMemory[currentImage]);
		}
	
	};
}