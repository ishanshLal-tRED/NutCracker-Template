#if NTKR_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>; // re include for leftover ones
#undef max
#undef min
import <glm/glm.hpp>;

export import <array>;
export import <set>;
export import <span>;
export import <string>;
export import <tuple>;

#include <Nutcracker/Core/Logging.hxx>;
export module NutCracker.Example.VulkanAPP:Context;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

export constexpr uint8_t VULKAN_VERTEX_BUFFER_BIND_ID = 0;
export constexpr uint8_t VULKAN_INSTANCE_BUFFER_BIND_ID = 1;

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static consteval 
	VkVertexInputBindingDescription getBindingDescription () {
		return VkVertexInputBindingDescription {
			.binding = VULKAN_VERTEX_BUFFER_BIND_ID,
			.stride = sizeof (Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
	}

	template<uint32_t StartFrom = 0> static consteval 
	std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions () {
		return { 
			VkVertexInputAttributeDescription {
				.location = StartFrom + 0,
				.binding = VULKAN_VERTEX_BUFFER_BIND_ID,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof (Vertex, pos),
			},
			VkVertexInputAttributeDescription {
				.location = StartFrom + 1,
				.binding = VULKAN_VERTEX_BUFFER_BIND_ID,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof (Vertex, color),
			},
		};
	}
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct VulkanContext {
#ifdef NDEBUG
	const bool EnableDebugging = false;
	bool EnableDebugMarkers = false;
#else
	const bool EnableDebugging = true;
	bool EnableDebugMarkers = true;
#endif

	static inline const std::array<const char*, 1> MinimumRequiredValidationLayers { 
		"VK_LAYER_KHRONOS_validation",
	};
	static inline const std::array<const char*, 2> MinimumRequiredInstanceExtensions { 
		VK_KHR_SURFACE_EXTENSION_NAME,
	#if NTKR_WINDOWS
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	#else
		static_assert (false, "not implemented")
	#endif
	};
	static inline const std::array<const char*, 1> MinimumRequiredDeviceExtensions { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	static inline const std::array<const char*, 1> OptionalDeviceExtensions { 
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
	};

	struct {
		// Set of instance extensions to be enabled for this example (must be set in the derived constructor)
		// also even though they seem to be dynamic, they constructed on your application requirement.
		std::set <std::string_view> EnabledInstanceExtensions;

		// Vulkan instance, stores all per-application states
		VkInstance Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT DebugMessenger;

		struct {
			std::set <std::string_view> EnabledExtensions;
			// Physical device (GPU) that Vulkan will use
			VkPhysicalDevice Handle = VK_NULL_HANDLE;
			// Stores physical device properties (for e.g. checking device limits)
			VkPhysicalDeviceProperties Properties;
			// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
			VkPhysicalDeviceFeatures Features;
			// Stores all available memory (type) properties for the physical device
			VkPhysicalDeviceMemoryProperties MemoryProperties;
		} PhysicalDevice;

		struct {
			VkSurfaceKHR Instance;
			VkSurfaceCapabilitiesKHR Capabilities (VkPhysicalDevice device) {
				// TODO: thi is a quick-fix current extent (max, min may) change(s) 
				VkSurfaceCapabilitiesKHR current_capabilities;
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Instance, &current_capabilities);
				return current_capabilities;
			};
			
			std::vector<VkSurfaceFormatKHR> Formats;
			std::vector<VkPresentModeKHR> PresentModes;
		} Surface; // should be converted to vector to support multiple displays

		// Depth buffer format (selected during Vulkan initialization)
		VkFormat DepthFormat = VK_FORMAT_UNDEFINED;

		union {
			struct {
				uint32_t Graphics, Presentation, Compute, Transfer;
			};	
			uint32_t as_arr [4];
		} QueueFamilyIndices;
	} Static;

	struct {
		// Set of physical device features to be enabled for this example (must be set in the derived constructor)
		VkPhysicalDeviceFeatures EnabledFeatures;

		// Logical device, application's view of the physical device (GPU)
		VkDevice LogicalDevice;
		// Handle to the device queues that command buffers are submitted to
		union {
			struct {
				VkQueue Graphics, Presentation, Compute, Transfer;
			};
			VkQueue as_arr [4];
		} Queues;

		struct {
			VkCommandPool Graphics = VK_NULL_HANDLE, Compute = VK_NULL_HANDLE, Transfer = VK_NULL_HANDLE;
		} CmdPools;

		struct {
			VkCommandBuffer Draw [MAX_FRAMES_IN_FLIGHT];
			VkCommandBuffer Compute;
			VkCommandBuffer Transfer;
		} CmdBuffers;

		struct {
			VkSwapchainKHR Instance = VK_NULL_HANDLE;

			std::vector<VkImage> Images;
			std::vector<VkImageView> ImagesView;

			uint32_t ImageCount; // framebuffer count
			VkFormat ImageFormat;
			VkColorSpaceKHR ColorSpace;
			VkExtent2D ImageExtent;
			VkPresentModeKHR PresentMode;
		} Swapchain;

		struct {
			VkImage Image;
			VkDeviceMemory ImageMem;
			VkImageView ImageView;
		} DepthStencil;

		VkSemaphore ImageAvailableSemaphores [MAX_FRAMES_IN_FLIGHT];
		VkSemaphore RenderFinishedSemaphores [MAX_FRAMES_IN_FLIGHT];

		// Note: fences can only be made after creation of cmd pool
		VkFence InFlightFences [MAX_FRAMES_IN_FLIGHT]; 

		VkRenderPass RenderPass;

		// List of output frame buffers
		std::vector<VkFramebuffer> Framebuffers;

		// Pipeline cache object
		VkPipelineCache PipelineCache;
	} Dynamic;

	struct {
// Specification:
// EVERYTHING INSIDE IS TARGETED and TO BE CONFIGURED ACCORDING TO PROGRAM REQUIREMENTS.
// Single SUBPASS based with vertex and fragment stages, 2 texture samplers etc.
		//struct {
		//	VkShaderModule Vertex;
		//	VkShaderModule Fragment;
		//} Modules;
		
		VkDescriptorPool DescriptorPool;
	
		struct {
			VkDescriptorSetLayout DescriptorSetLayout;

			VkDescriptorSet DescriptorSets [MAX_FRAMES_IN_FLIGHT];
		} Configs;

		VkPipelineLayout Layout;
		VkPipeline Instance;
	} Pipeline;
}; // m_Vk

struct VulkanScene {
	VkBuffer VertexBuffer;
	VkDeviceMemory VertexBufferMemory;
	VkBuffer IndexBuffer;
	VkDeviceMemory IndexBufferMemory;

	VkBuffer UniformBuffers [MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory UniformBuffersMemory [MAX_FRAMES_IN_FLIGHT];
}; // m_VkScene

namespace Hlpr {
	uint32_t FindVkMemoryType (const VkPhysicalDeviceMemoryProperties &mem_properties, const uint32_t type_filter, const VkMemoryPropertyFlags properties) {
		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
			if ((type_filter & (1 << i)) && 
				 ((mem_properties.memoryTypes[i].propertyFlags & properties) == properties) )
				return i;
		
		THROW_critical ("failed to find suitable memory type");
	}

	void AllocateCommandBuffers (std::span<VkCommandBuffer> command_buffers, const VkDevice device, const VkCommandPool command_pool) {
		VkCommandBufferAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = uint32_t (command_buffers.size ()),
		};
	
		vkAllocateCommandBuffers (device, &alloc_info, command_buffers.data ());
	}
	
	void BeginSingleTimeVkCommands (const VkCommandBuffer command_buffer)
	{
		vkResetCommandBuffer (command_buffer, 0);
	
		VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
	
		vkBeginCommandBuffer (command_buffer, &begin_info);
	}
	
	void EndSingleVkCommand (const VkQueue queue, const VkCommandBuffer command_buffer)
	{
		vkEndCommandBuffer (command_buffer);
	
		VkSubmitInfo submit_info {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &command_buffer
		};
		vkQueueSubmit (queue, 1, &submit_info, VK_NULL_HANDLE);
	
		vkQueueWaitIdle (queue);
	}
	
	void CreateVkBuffer (VkBuffer& buffer, VkDeviceMemory& buffer_memory
		,const VkDevice device, const VkPhysicalDevice physical_device, const VkDeviceSize size
		,const VkBufferUsageFlags usage, const VkPhysicalDeviceMemoryProperties &device_mem_properties, const VkMemoryPropertyFlags properties, const void* data_ptr = nullptr)
	{
		VkBufferCreateInfo buffer_info {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
	
		if (vkCreateBuffer (device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) 
			THROW_critical ("failed to create buffer!");
		
	
		VkMemoryRequirements mem_requirements;
		vkGetBufferMemoryRequirements (device, buffer, &mem_requirements);
	
		VkMemoryAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_requirements.size,
			.memoryTypeIndex = FindVkMemoryType (device_mem_properties, mem_requirements.memoryTypeBits, properties)
		};
	
		if (vkAllocateMemory (device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) 
			THROW_critical ("failed to allocate buffer memory!");
		
	
		vkBindBufferMemory (device, buffer, buffer_memory, 0);
	
		if (data_ptr) { 
			if (! (properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
				THROW_critical ("data can only be mapped to HOST_VISIBLE location");

			void* device_buffer_ptr = nullptr;
			vkMapMemory (device, buffer_memory, 0, size, 0, &device_buffer_ptr);
			memcpy (device_buffer_ptr, data_ptr, size);
			vkUnmapMemory (device, buffer_memory);
		}
	};

	void TransitionVkImageLayout (VkCommandBuffer cmd_buf
		,VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range
		,VkPipelineStageFlags source_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		,VkPipelineStageFlags destination_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) 
	{
		VkImageMemoryBarrier barrier {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

			.srcAccessMask = 0, // Later
			.dstAccessMask = 0, // Later

			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = subresource_range,
		};

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (old_layout)
		{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				barrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (new_layout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (barrier.srcAccessMask == 0)
				{
					barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
		}

		vkCmdPipelineBarrier (cmd_buf
			,source_stage_mask, destination_stage_mask
			,0
			,0, nullptr
			,0, nullptr
			,1, &barrier);
	}

	/// <summary>
	/// Creates an image on GPU, in uint8_t* data format
	/// </summary>
	/// <param name="image">out image handle for image</param>
	/// <param name="image_memory">out allocated memory for image</param>
	/// <param name="image_view">out image view of VkImage</param>
	/// <param name="format">VkFormat of image</param>
	/// <param name="image_dimensions">full image dimensions (with mips) VkExtent3D{image width, height, channels}</param>
	/// <param name="mips">array of <{offset:uint32, VkExtent2D{width:int32, height:int32}}</param>
	/// <param name="device">logical/application view of GPU</param>
	/// <param name="physical_device">the GPU</param>
	/// <param name="transfer_queue"></param>
	/// <param name="cmd_pool"></param>
	/// <param name="device_mem_properties">physical device memory properties</param>
	/// <param name="device_properties">physical device properties</param>
	/// <param name="device_features">physical device features</param>
	/// <param name="mem_property_flags"></param>
	/// <param name="data_ptr">image data of full image</param>
	/// <param name="image_layout">optional, layout of image</param>
	/// <param name="usage">optional, image usage</param>
	/// <param name="sampler">optional out, when provided creates default sampler</param>
	/// <param name="force_linear_tiling">optional, force use of linear tiling for image, not advised</param>
	void CreateVkImage (VkImage& image, VkDeviceMemory& image_memory, VkImageView& image_view
		,const VkFormat format, const VkExtent3D image_dimensions, const std::span<std::pair<uint32_t, VkExtent2D>> mips
		,const VkDevice device, const VkPhysicalDevice physical_device, const VkQueue transfer_queue, const VkCommandPool cmd_pool 
		,const VkPhysicalDeviceMemoryProperties& device_mem_properties
		,const VkPhysicalDeviceProperties& device_properties, const VkPhysicalDeviceFeatures device_features 
		,const VkMemoryPropertyFlags mem_property_flags, const void* data_ptr
		,const VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		,const VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT
		,VkSampler* sampler = nullptr, bool force_linear_tiling = false)
	{
		VkFormatProperties format_props;
		vkGetPhysicalDeviceFormatProperties (physical_device, format, &format_props);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 use_staging = !force_linear_tiling;

		VkMemoryRequirements mem_requirements;
		VkMemoryAllocateInfo mem_alloc_info;

		std::array<VkCommandBuffer, 1> cpy_cmd_buff;
		AllocateCommandBuffers (cpy_cmd_buff, device, cmd_pool);

		if (use_staging && !force_linear_tiling) { // aka optimal tiling
			struct {
				VkBuffer buf;
				VkDeviceMemory mem;
			} staging;

			VkDeviceSize buffer_size = image_dimensions.width*image_dimensions.height*image_dimensions.depth;
			CreateVkBuffer (staging.buf, staging.mem, device, physical_device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, device_mem_properties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data_ptr);

			// each mip level
			std::vector<VkBufferImageCopy> buffer_cpy_regions (mips.size ());
			for (uint32_t mip_level = 0; auto& cpy_region: buffer_cpy_regions){
				cpy_region = VkBufferImageCopy {
					.bufferOffset = mips[mip_level].first,

					.imageSubresource = VkImageSubresourceLayers {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
						.mipLevel = mip_level,
						.baseArrayLayer = 0, 
						.layerCount = 1
					},
					
					.imageOffset = {0,0,0},
					.imageExtent = {mips[mip_level].second.width, mips[mip_level].second.height, 1},
				};

				mip_level++;
			}

			VkImageCreateInfo image_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = VkExtent3D {image_dimensions.width, image_dimensions.height, 1},
				.mipLevels = uint32_t (mips.size ()),
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};

			if (vkCreateImage (device, &image_info, nullptr, &image) != VK_SUCCESS)
				THROW_critical ("failed to create image!");
		
			// VkMemoryRequirements mem_requirements;
			vkGetImageMemoryRequirements (device, image, &mem_requirements);
		
			// VkMemoryAllocateInfo mem_alloc_info;
			mem_alloc_info = VkMemoryAllocateInfo {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = mem_requirements.size,
				.memoryTypeIndex = FindVkMemoryType (device_mem_properties, mem_requirements.memoryTypeBits, mem_property_flags)
			};
		
			if (vkAllocateMemory (device, &mem_alloc_info, nullptr, &image_memory) != VK_SUCCESS)
				THROW_critical ("failed to allocate buffer memory!");
		
			if (vkBindImageMemory (device, image, image_memory, 0) != VK_SUCCESS)
				THROW_critical ("failed to allocate buffer memory!");

			VkImageSubresourceRange subresource_range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = uint32_t (mips.size ()),
				.layerCount = 1,
			};

			BeginSingleTimeVkCommands (cpy_cmd_buff[0]);
			TransitionVkImageLayout (cpy_cmd_buff[0]
				,image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
			EndSingleVkCommand (transfer_queue, cpy_cmd_buff[0]);
			
			BeginSingleTimeVkCommands (cpy_cmd_buff[0]);
			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage (
				cpy_cmd_buff[0],
				staging.buf,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				uint32_t (buffer_cpy_regions.size ()),
				buffer_cpy_regions.data ()
			);
			EndSingleVkCommand (transfer_queue, cpy_cmd_buff[0]);
			
			BeginSingleTimeVkCommands (cpy_cmd_buff[0]);
			// Change texture image layout to shader read after all mip levels have been copied
			TransitionVkImageLayout (cpy_cmd_buff[0]
				,image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_layout, subresource_range);
			EndSingleVkCommand (transfer_queue, cpy_cmd_buff[0]);
			
			// Clean up staging resources
			vkFreeMemory (device, staging.mem, nullptr);
			vkDestroyBuffer (device, staging.buf, nullptr);
		} else {
			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			// Check if this support is supported for linear tiling
			if (! (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) 
				THROW_critical ("No support for Linear tiling");

			struct {
				VkImage img;
				VkDeviceMemory mem;
			} mappable;

			VkImageCreateInfo image_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = {image_dimensions.width, image_dimensions.height, 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_LINEAR,
				.usage = usage,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};

			if (vkCreateImage (device, &image_info, nullptr, &mappable.img) != VK_SUCCESS)
				THROW_critical ("failed to create image!");
			
			vkGetImageMemoryRequirements (device, mappable.img, &mem_requirements);
			
			mem_alloc_info = VkMemoryAllocateInfo {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = mem_requirements.size,
				.memoryTypeIndex = FindVkMemoryType (device_mem_properties, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			};
			
			
			if (vkAllocateMemory (device, &mem_alloc_info, nullptr, &mappable.mem) != VK_SUCCESS)
				THROW_critical ("failed to allocate buffer memory!");
		
			if (vkBindImageMemory (device, mappable.img, mappable.mem, 0) != VK_SUCCESS)
				THROW_critical ("failed to allocate buffer memory!");

			VkImageSubresource subresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
			};

			VkSubresourceLayout subresource_layout;
			void* device_buffer_ptr = nullptr;

			// Map image memory
			vkMapMemory (device, mappable.mem, 0, mem_requirements.size, 0, &device_buffer_ptr);
			memcpy (device_buffer_ptr, data_ptr, mem_requirements.size);
			vkUnmapMemory (device, mappable.mem);

			// Linear tiled images don't need to be staged
			// and can be directly used as textures
			image = mappable.img;
			image_memory = mappable.mem;

			VkImageSubresourceRange subresource_range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.layerCount = 1,
			};

			BeginSingleTimeVkCommands (cpy_cmd_buff[0]);
			// Change texture image layout to shader read after all mip levels have been copied
			TransitionVkImageLayout (cpy_cmd_buff[0]
				,image, VK_IMAGE_LAYOUT_UNDEFINED, image_layout, subresource_range);
			EndSingleVkCommand (transfer_queue, cpy_cmd_buff[0]);
		}

		if (sampler) {
			VkSamplerCreateInfo sampler_info {
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,

				.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,

				.mipLodBias = 0.0f,

				// should be enabled only if anisotropy is enabled on this device ;-3
				.anisotropyEnable = device_features.samplerAnisotropy,
				.maxAnisotropy = device_features.samplerAnisotropy ? device_properties.limits.maxSamplerAnisotropy : 1.0f,

				.compareEnable = VK_FALSE,
				.compareOp = VK_COMPARE_OP_ALWAYS, // _NEVER,

				.minLod = 0.0f,
				.maxLod = (force_linear_tiling) ? 0.0f : float (mips.size ()),
				.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
				.unnormalizedCoordinates = VK_FALSE,
			};

			if (vkCreateSampler (device, &sampler_info, nullptr, sampler) != VK_SUCCESS) 
				THROW_critical ("failed to create texture sampler");
		}

		// image view for texture image
		VkImageViewCreateInfo view_info {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
				.baseMipLevel = 0, 
				.levelCount = 1,
				.baseArrayLayer = 0, 
				.layerCount = 1 
			}
		};
		if (vkCreateImageView (device, &view_info, nullptr, &image_view) != VK_SUCCESS) 
			THROW_critical ("failed to create textureimage view!");
	};

	VkShaderModule CreateVkShaderModule (const VkDevice device, const std::vector<char>& byte_code) {
		VkShaderModuleCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = byte_code.size (),
			.pCode = (const uint32_t*) (byte_code.data ()) // should be aligned as (uint32_t);
		};

		VkShaderModule shader_module;
		if (vkCreateShaderModule (device, &create_info, nullptr, &shader_module) != VK_SUCCESS) 
			THROW_critical ("failed to create shader module!");
		return shader_module;
	};
};