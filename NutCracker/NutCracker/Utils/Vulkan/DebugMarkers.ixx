export module Nutcracker.Utils.Vulkan.DebugMarkers;

import <vulkan/vulkan.h>;
import <glm/glm.hpp>;
import <string>;

namespace NutCracker {
	// Setup and functions for the VK_EXT_debug_marker_extension
	// Extension spec can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
	// Note that the extension will only be present if run from an offline debugging application
	// The actual check for extension presence and enabling it on the device is done in the example base class
	export 
	namespace DebugMarker {
		// Set to true if function pointer for the debug marker are available
		extern bool active;

		// Get function pointers for the debug report extensions from the device
		void setup (VkDevice device);

		// Sets the debug name of an object
		// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
		// along with the object type
		void setObjectName (VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name);

		// Set the tag for an object
		void setObjectTag (VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);

		// Start a new debug marker region
		void beginRegion (VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color);

		// Insert a new debug marker into the command buffer
		void insert (VkCommandBuffer cmdbuffer, std::string_view markerName, glm::vec4 color);

		// End the current debug marker region
		void endRegion (VkCommandBuffer cmdBuffer);

		// Object specific naming functions
		void setCommandBufferName		(VkDevice device, VkCommandBuffer cmdBuffer, const char * name);
		void setQueueName				(VkDevice device, VkQueue queue, const char * name);
		void setImageName				(VkDevice device, VkImage image, const char * name);
		void setSamplerName				(VkDevice device, VkSampler sampler, const char * name);
		void setBufferName				(VkDevice device, VkBuffer buffer, const char * name);
		void setDeviceMemoryName		(VkDevice device, VkDeviceMemory memory, const char * name);
		void setShaderModuleName		(VkDevice device, VkShaderModule shaderModule, const char * name);
		void setPipelineName			(VkDevice device, VkPipeline pipeline, const char * name);
		void setPipelineLayoutName		(VkDevice device, VkPipelineLayout pipelineLayout, const char * name);
		void setRenderPassName			(VkDevice device, VkRenderPass renderPass, const char * name);
		void setFramebufferName			(VkDevice device, VkFramebuffer framebuffer, const char * name);
		void setDescriptorSetLayoutName	(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name);
		void setDescriptorSetName		(VkDevice device, VkDescriptorSet descriptorSet, const char * name);
		void setSemaphoreName			(VkDevice device, VkSemaphore semaphore, const char * name);
		void setFenceName				(VkDevice device, VkFence fence, const char * name);
		void setEventName				(VkDevice device, VkEvent _event, const char * name);
	};
};

bool active = false;

PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = VK_NULL_HANDLE;
PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = VK_NULL_HANDLE;

void NutCracker::DebugMarker::setup(VkDevice device)
{
	pfnDebugMarkerSetObjectTag = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT"));
	pfnDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT"));
	pfnCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT"));
	pfnCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT"));
	pfnCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT"));

	// Set flag if at least one function pointer is present
	active = (pfnDebugMarkerSetObjectName != VK_NULL_HANDLE);
}

void NutCracker::DebugMarker::setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (pfnDebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name;
		pfnDebugMarkerSetObjectName(device, &nameInfo);
	}
}

void NutCracker::DebugMarker::setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (pfnDebugMarkerSetObjectTag)
	{
		VkDebugMarkerObjectTagInfoEXT tagInfo = {};
		tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
		tagInfo.objectType = objectType;
		tagInfo.object = object;
		tagInfo.tagName = name;
		tagInfo.tagSize = tagSize;
		tagInfo.pTag = tag;
		pfnDebugMarkerSetObjectTag(device, &tagInfo);
	}
}

void NutCracker::DebugMarker::beginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (pfnCmdDebugMarkerBegin)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = pMarkerName;
		pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
	}
}

void NutCracker::DebugMarker::insert(VkCommandBuffer cmdbuffer, std::string_view markerName, glm::vec4 color)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (pfnCmdDebugMarkerInsert)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = markerName.c_str();
		pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
	}
}

void NutCracker::DebugMarker::endRegion(VkCommandBuffer cmdBuffer)
{
	// Check for valid function (may not be present if not running in a debugging application)
	if (pfnCmdDebugMarkerEnd)
	{
		pfnCmdDebugMarkerEnd(cmdBuffer);
	}
}

void NutCracker::DebugMarker::setCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char * name)
{
	setObjectName(device, (uint64_t)cmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, name);
}

void NutCracker::DebugMarker::setQueueName(VkDevice device, VkQueue queue, const char * name)
{
	setObjectName(device, (uint64_t)queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, name);
}

void NutCracker::DebugMarker::setImageName(VkDevice device, VkImage image, const char * name)
{
	setObjectName(device, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
}

void NutCracker::DebugMarker::setSamplerName(VkDevice device, VkSampler sampler, const char * name)
{
	setObjectName(device, (uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, name);
}

void NutCracker::DebugMarker::setBufferName(VkDevice device, VkBuffer buffer, const char * name)
{
	setObjectName(device, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
}

void NutCracker::DebugMarker::setDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char * name)
{
	setObjectName(device, (uint64_t)memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name);
}

void NutCracker::DebugMarker::setShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char * name)
{
	setObjectName(device, (uint64_t)shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, name);
}

void NutCracker::DebugMarker::setPipelineName(VkDevice device, VkPipeline pipeline, const char * name)
{
	setObjectName(device, (uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, name);
}

void NutCracker::DebugMarker::setPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char * name)
{
	setObjectName(device, (uint64_t)pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, name);
}

void NutCracker::DebugMarker::setRenderPassName(VkDevice device, VkRenderPass renderPass, const char * name)
{
	setObjectName(device, (uint64_t)renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, name);
}

void NutCracker::DebugMarker::setFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char * name)
{
	setObjectName(device, (uint64_t)framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, name);
}

void NutCracker::DebugMarker::setDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name)
{
	setObjectName(device, (uint64_t)descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, name);
}

void NutCracker::DebugMarker::setDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char * name)
{
	setObjectName(device, (uint64_t)descriptorSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, name);
}

void NutCracker::DebugMarker::setSemaphoreName(VkDevice device, VkSemaphore semaphore, const char * name)
{
	setObjectName(device, (uint64_t)semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, name);
}

void NutCracker::DebugMarker::setFenceName(VkDevice device, VkFence fence, const char * name)
{
	setObjectName(device, (uint64_t)fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, name);
}

void NutCracker::DebugMarker::setEventName(VkDevice device, VkEvent _event, const char * name)
{
	setObjectName(device, (uint64_t)_event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, name);
}