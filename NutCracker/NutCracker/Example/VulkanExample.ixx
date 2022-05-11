import NutCracker.Base.AppInstance;

export import <vulkan/vulkan.h>;
import <string>;
import <set>;

#include <Core/Logging.hxx>;
#include <Utils/HelperMacros.hxx>;

import NutCracker.Base.Window;
import NutCracker.Events;

export module NutCracker.Example.VulkanExample;

export struct VulkanContext; // see below/lower part of file
namespace NutCracker::Example {
	export
	class VulkanExample: public ::NutCracker::BaseInstance {
	public:
		void setup (const std::span<char*> argument_list) override { 
			LOG_trace(std::source_location::current ().function_name ());
			m_Window = Window::Create (WindowProps {
				.Title = "Vulkan Sandbox 1",
				.Width = 1280,
				.Height = 720,
			});
			m_Window->SetEventCallback (NTKR_BIND_FUNCTION (onEvent));
		}

		void initializeVk () override;

		void update (double update_latency) override 
			{ /* LOG_trace(std::source_location::current ().function_name ()); */ m_Window->Update(); }

		void render (double render_latency) override;

		void terminateVk () override;

		void cleanup () override 
			{ LOG_trace(std::source_location::current ().function_name ()); delete m_Window; }

		void onEvent (Event& e) override {
			EventDispatcher dispatcher(e);
			dispatcher.Dispatch<WindowCloseEvent>(NTKR_BIND_FUNCTION(onWndClose));
		}
		bool onWndClose (WindowCloseEvent& e) {
			m_Running = false;
			return true;
		}
		bool keepContextRunning () override { return m_Running; }
	protected:
		void createInstance ();
		void setupDebugging (VkDebugUtilsMessageSeverityFlagBitsEXT);
		void createSurface (); // surface of your window
		void pickPhysicalDevice ();
		void getEnabledFeatures ();
		void createLogicalDevice (const VkQueueFlagBits);
		void getRequiredQueues (); // can be(s) graphics, presentation, compute, transfer VkQueues
		void createSyncronisationObjects (); // aka semaphores and not fences

		void prepare ();
		void initSwapChain ();
		void createCommandPool (const VkQueueFlagBits);
		void setupSwapChain ();
		void createCommandBuffers ();
		void createSynchronizationPrimitives (); // & objects
		uint32_t findMemoryType (uint32_t type_filter, VkMemoryPropertyFlags properties);
		void setupDepthStencil ();
		void setupRenderPass (const bool add_depth = false);
		void createPipelineCache ();
		void setupFrameBuffer (const bool has_depth = false);

		void submitFrame ();

		void cleanupSwapchain ();
		void destroyCommandBuffers ();
	private:
		bool m_Running = true;
		uint32_t CurrentFrame = 0;
		Window* m_Window = nullptr;
		VulkanContext m_Vk;
	};
};

export constexpr uint32_t MAX_FRAMES_FLIGHT = 2;
struct VulkanContext {
	const bool EnableDebugging = true;
	const bool EnableDebugMarkers = true;
	
	const char* const DefaultRequiredValidationLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};
	const char* const DefaultRequiredDeviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	const char* const OptionalDeviceExtensions[] = {
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
	};

	struct {
		// Vulkan instance, stores all per-application states
		VkInstance Instance;
		std::vector <std::string> SuppotedExtensions;
		VkDebugUtilsMessengerEXT DebugMessenger;
		// should be converted to vector to support multiple displays
		struct {
			VkSurfaceKHR Instance;
			VkSurfaceCapabilitiesKHR Capabilities;
			std::set<VkSurfaceFormatKHR> Formats;
			std::set<VkPresentModeKHR> PresentModes;
		} Surface;
		// Physical device (GPU) that Vulkan will use
		VkPhysicalDevice PhysicalDevice;
		// Stores physical device properties (for e.g. checking device limits)
		VkPhysicalDeviceProperties PhysicalProperties;
		// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
		VkPhysicalDeviceFeatures PhysicalFeatures;
		// Stores all available memory (type) properties for the physical device
		VkPhysicalDeviceMemoryProperties PhysicalMemoryProperties;
		// Set of device extensions to be enabled for this example (must be set in the derived constructor)
		// also even though they seem to be dynamic, they constructed on your application requirement.
		std::set <std::string_view> EnabledInstanceExtensions;
		std::set <std::string_view> EnabledDeviceExtensions;
	
		// Handle to the device queues that command buffers are submitted to
		union {
			struct {
				VkQueue Graphics, Presentation, Compute, Transfer;
			} Queues;
				
			VkQueue Queue [4];
		}; 
		union {
			struct {
				uint32_t Graphics, Presentation, Compute, Transfer;
			} QueueFamilyIndices;
				
			uint32_t QueueFamilyIndexes [4];
		};
		// Depth buffer format (selected during Vulkan initialization)
		VkFormat DepthFormat;

		VkSemaphore ImageAvailableSemaphores [MAX_FRAMES_IN_FLIGHT];
		VkSemaphore RenderFinishedSemaphores [MAX_FRAMES_IN_FLIGHT];
	} Static; // static across app instance

	struct {
		// Set of physical device features to be enabled for this example (must be set in the derived constructor)
		VkPhysicalDeviceFeatures EnabledFeatures;
		// Logical device, application's view of the physical device (GPU)
		VkDevice LogicalDevice;
		// equal to no. of queues
		VkCommandPool CmdPool;
		// fences can only be made after creation of cmd pool
		VkFence InFlightFences [MAX_FRAMES_IN_FLIGHT]; 
		// Pipeline cache object
		VkPipelineCache PipelineCache;
		// Pipeline stages used to wait at for graphics queue submissions
		VkPipelineStageFlags SubmitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// Contains command buffers and semaphores to be presented to the queue
		VkSubmitInfo SubmitInfo;
		// Command buffers used for rendering
		VkCommandBuffer DrawCmdBuffers [MAX_FRAMES_IN_FLIGHT];
		// Global render pass for frame buffer writes
		VkRenderPass RenderPass = VK_NULL_HANDLE;

		struct {
			VkImage Image;
			VkDeviceMemory ImageMem;
			VkImageView ImageView;
		} DepthStencil;
		struct {
			VkSwapchainKHR Instance;
			std::vector<VkImage> Images;
			std::vector<VkImageView> ImagesView;
			VkFormat ImageFormat;
			VkColorSpaceKHR ColorSpace;
			VkExtent2D ImageExtent;
		} Swapchain;
		// List of available frame buffers (same as number of swapchain images)
		std::vector<VkFramebuffer> Framebuffers;

		// Descriptor set pool
		VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
		// List of shader modules created (stored for cleanup)
		std::vector<VkShaderModule> ShaderModules;
		
	} Dynamic;
};