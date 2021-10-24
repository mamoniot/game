//By Monica Moniot
#ifdef DEBUG
#define MAMLIB_DEBUG
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define MAMLIB_IMPLEMENTATION
#include "mamlib.h"
#define PCG_IMPLEMENTATION
#include "pcg.h"
#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"
#include "basic.h"
#include "SDL.h"
#include "SDL_vulkan.h"
#include "vulkan/vulkan.h"
#undef main

#include "types.hh"
#include "config.hh"

#define min gb_min
#define max gb_max


static MamString read_file_to_stack(const char* filename, MamStack* stack) {
	SDL_RWops* file = SDL_RWFromFile(filename, "r");
	if(!file) {
		const char* error = SDL_GetError();
		char str[512] = {};
		snprintf(str, 512, "Could not find critical file: %s; SDL Error: %s\n", filename, error);
		MAM_ERRORL(str);
	}
	int32 size = SDL_RWsize(file);
	char* buffer = mam_stack_pusht(char, stack, size);
	SDL_RWread(file, buffer, 1, size);
	SDL_RWclose(file);
	return mam_memtostr(buffer, size);
}

void game_free_recursively(GameMemDesc* desc);


static double get_delta_time(uint64 t0, uint64 t1) {
	return (t1 - t0)/cast(double, SDL_GetPerformanceFrequency());
}


void main_cleanup(MainTrash* data) {
	{//clean up vulkan
		MvkData* mvk = data->mvk;
		if(mvk->device) vkDeviceWaitIdle(mvk->device);
		if(mvk->image_available_sems) {
			for_each_in(VkSemaphore, sem, mvk->image_available_sems, MVK_FRAMES_IN_FLIGHT) vkDestroySemaphore(mvk->device, *sem, 0);
			for_each_in(VkSemaphore, sem, mvk->render_finished_sems, MVK_FRAMES_IN_FLIGHT) vkDestroySemaphore(mvk->device, *sem, 0);
			for_each_in(VkFence, fence, mvk->in_flight_fences, MVK_FRAMES_IN_FLIGHT) vkDestroyFence(mvk->device, *fence, 0);
		}
		if(mvk->command_pool) vkDestroyCommandPool(mvk->device, mvk->command_pool, 0);
		if(mvk->frame_buffers) {
			for_each_in(VkFramebuffer, frame_buffer, mvk->frame_buffers, mvk->swap_chain_size) vkDestroyFramebuffer(mvk->device, *frame_buffer, 0);
		}
		if(mvk->pipeline) vkDestroyPipeline(mvk->device, mvk->pipeline, 0);
		if(mvk->pipeline_layout) vkDestroyPipelineLayout(mvk->device, mvk->pipeline_layout, 0);
		if(mvk->render_pass) vkDestroyRenderPass(mvk->device, mvk->render_pass, 0);
		for_each_in(VkPipelineShaderStageCreateInfo, shader_stage, mvk->shader_stages, mvk->shader_stages_size) vkDestroyShaderModule(mvk->device, shader_stage->module, 0);
		for_each_in(VkImageView, image_view, mvk->swap_chain_image_views, mvk->swap_chain_size) vkDestroyImageView(mvk->device, *image_view, 0);
		if(mvk->swap_chain) vkDestroySwapchainKHR(mvk->device, mvk->swap_chain, 0);
		if(mvk->descriptor_set_layout) vkDestroyDescriptorSetLayout(mvk->device, mvk->descriptor_set_layout, 0);
		if(mvk->vertex_buffer) vkDestroyBuffer(mvk->device, mvk->vertex_buffer, 0);
		if(mvk->vertex_buffer_memory) vkFreeMemory(mvk->device, mvk->vertex_buffer_memory, 0);
		if(mvk->index_buffer) vkDestroyBuffer(mvk->device, mvk->index_buffer, 0);
		if(mvk->index_buffer_memory) vkFreeMemory(mvk->device, mvk->index_buffer_memory, 0);
		if(mvk->uniform_buffer) vkDestroyBuffer(mvk->device, mvk->uniform_buffer, 0);
		if(mvk->uniform_buffer_memory) vkFreeMemory(mvk->device, mvk->uniform_buffer_memory, 0);
		if(mvk->descriptor_pool) vkDestroyDescriptorPool(mvk->device, mvk->descriptor_pool, 0);
		if(mvk->surface) vkDestroySurfaceKHR(mvk->instance, mvk->surface, 0);
		if(mvk->device) vkDestroyDevice(mvk->device, 0);
		if(mvk->instance) vkDestroyInstance(mvk->instance, 0);
	}
	if(data->window) SDL_DestroyWindow(data->window);
	if(data->sdl_isinit) SDL_Quit();
	#ifdef DEBUG
	if(data->game_desc.mem) game_free_recursively(&data->game_desc);
	for_each_in(void*, ptr, data->ptrs, TRASH_PTRS_SIZE) {
		if(*ptr) free(*ptr);
	}
	#endif
}

void main_trap(void* data) {
	main_cleanup((MainTrash*)data);
	//TODO: improve graceful exit of program
	mam_system_error_trap(0);
}


void create_buffer(MvkData* mvk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory) {
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(mvk->device, &buffer_info, nullptr, buffer) != VK_SUCCESS) {
        ERRORL("Failed to create a vulkan buffer\n");
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(mvk->device, *buffer, &memory_requirements);

	uint32 filter = memory_requirements.memoryTypeBits;
	int32 mem_type_i = -1;
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(mvk->physical_device, &memory_properties);

		for_each_lt(i, memory_properties.memoryTypeCount) {
			if((filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				mem_type_i = i;
				break;
			}
		}
		if(mem_type_i < 0) {
			ERRORL("Failed to find suitable memory type for a vulkan buffer\n");
		}
	}

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = mem_type_i;

    if (vkAllocateMemory(mvk->device, &alloc_info, 0, buffer_memory) != VK_SUCCESS) {
        ERRORL("Failed to allocate memory for a vulkan buffer\n");
    }

    vkBindBufferMemory(mvk->device, *buffer, *buffer_memory, 0);
}

void copy_buffer(MvkData* mvk, VkBuffer buffer_dst, VkBuffer buffer_src, VkDeviceSize size) {//copy buffer
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = mvk->command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(mvk->device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0; // Optional
	copy_region.dstOffset = 0; // Optional
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, buffer_src, buffer_dst, 1, &copy_region);

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(mvk->draw_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(mvk->draw_queue);

	vkFreeCommandBuffers(mvk->device, mvk->command_pool, 1, &command_buffer);
}

void find_device_capabilities(MvkData* mvk, SDL_Window* window) {
	int32 pre_stack_size = mvk->stack->size;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mvk->physical_device, mvk->surface, &mvk->capabilities);

	int width;
	int height;
	SDL_Vulkan_GetDrawableSize(window, &width, &height);

	mvk->swap_chain_image_extent.width = gb_clamp(width, mvk->capabilities.minImageExtent.width, mvk->capabilities.maxImageExtent.width);
	mvk->swap_chain_image_extent.height = gb_clamp(height, mvk->capabilities.minImageExtent.height, mvk->capabilities.maxImageExtent.height);
	// if(mvk->swap_chain_image_extent.width == MAX_UINT32)

	mvk->swap_chain_size = mvk->capabilities.minImageCount + 1;
	if(mvk->capabilities.maxImageCount > 0) {
		mvk->swap_chain_size = min(mvk->swap_chain_size, mvk->capabilities.maxImageCount);
	}
	/*
	VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may result in tearing.
	VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue. If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is refreshed is known as "vertical blank".
	VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
	VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to implement triple buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync that uses double buffering.
	*/
	uint32 present_modes_size = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(mvk->physical_device, mvk->surface, &present_modes_size, 0);
	VkPresentModeKHR* present_modes = mam_stack_pusht(VkPresentModeKHR, mvk->stack, present_modes_size);
	vkGetPhysicalDeviceSurfacePresentModesKHR(mvk->physical_device, mvk->surface, &present_modes_size, present_modes);

	mvk->present_mode = VK_PRESENT_MODE_FIFO_KHR;//guaranteed to be available
	for_each_in(VkPresentModeKHR, mode, present_modes, present_modes_size) {
		if(*mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			mvk->present_mode = *mode;
			break;
		}
		#if PEDAL_TO_THE_METAL
		if(*mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			mvk->present_mode = *mode;
			break;
		}
		#endif
	}
	mvk->device_does_vsync = mvk->present_mode == VK_PRESENT_MODE_FIFO_KHR || mvk->present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR;

	uint32 formats_size = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mvk->physical_device, mvk->surface, &formats_size, 0);
	VkSurfaceFormatKHR* formats = mam_stack_pusht(VkSurfaceFormatKHR, mvk->stack, formats_size);
	vkGetPhysicalDeviceSurfaceFormatsKHR(mvk->physical_device, mvk->surface, &formats_size, formats);

	mvk->surface_format = formats[0];
	for_each_in(VkSurfaceFormatKHR, format, formats, formats_size) {
		if (format->format == VK_FORMAT_B8G8R8A8_SRGB && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			mvk->surface_format = *format;
			break;
		}
	}

	mam_stack_set_size(mvk->stack, pre_stack_size);
}

void create_swap_chain(MvkData* mvk) {
	mvk->swap_chain_mem_start = mvk->stack->size;
	/*
	It is also possible that you'll render images to a separate image first to perform operations like post-processing. In that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap chain image.
	*/
	VkSwapchainCreateInfoKHR swap_info = {};
	swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_info.surface = mvk->surface;
	swap_info.imageExtent = mvk->swap_chain_image_extent;
	swap_info.minImageCount = mvk->swap_chain_size;
	swap_info.imageFormat = mvk->surface_format.format;
	swap_info.imageColorSpace = mvk->surface_format.colorSpace;
	swap_info.imageArrayLayers = 1;
	swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if(mvk->draw_queue_i == mvk->present_queue_i) {
		swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	} else {
		swap_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swap_info.queueFamilyIndexCount = 2;
		uint32 indices[2];
		indices[0] = mvk->draw_queue_i;
		indices[1] = mvk->present_queue_i;
		swap_info.pQueueFamilyIndices = indices;
	}
	swap_info.preTransform = mvk->capabilities.currentTransform;
	swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_info.presentMode = mvk->present_mode;
	swap_info.clipped = VK_TRUE;
	swap_info.oldSwapchain = VK_NULL_HANDLE;

	if(vkCreateSwapchainKHR(mvk->device, &swap_info, 0, &mvk->swap_chain) != VK_SUCCESS) {
		MAM_ERRORL("Failed to create vulkan swap chain\n");
	}


	vkGetSwapchainImagesKHR(mvk->device, mvk->swap_chain, &mvk->swap_chain_size, 0);
	VkImage* swap_chain_images = mam_stack_pusht(VkImage, mvk->stack, mvk->swap_chain_size);
	vkGetSwapchainImagesKHR(mvk->device, mvk->swap_chain, &mvk->swap_chain_size, swap_chain_images);

	mvk->swap_chain_image_views = mam_stack_pusht(VkImageView, mvk->stack, mvk->swap_chain_size);
	for_each_lt(i, mvk->swap_chain_size) {
		VkImageViewCreateInfo image_view_info = {};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = swap_chain_images[i];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = mvk->surface_format.format;
		image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		if(vkCreateImageView(mvk->device, &image_view_info, 0, &mvk->swap_chain_image_views[i]) != VK_SUCCESS) {
			MAM_ERRORL("Failed to create an image view to the vulkan swap chain\n");
		}
	}

	{//create uniform buffer
		VkDeviceSize buffer_size = mvk->swap_chain_size*sizeof(UniformBufferObject);

		create_buffer(mvk, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &mvk->uniform_buffer, &mvk->uniform_buffer_memory);

		VkDescriptorPoolSize pool_size_info = {};
		pool_size_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_size_info.descriptorCount = mvk->swap_chain_size;

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes = &pool_size_info;
		pool_info.maxSets = mvk->swap_chain_size;

		if(vkCreateDescriptorPool(mvk->device, &pool_info, 0, &mvk->descriptor_pool) != VK_SUCCESS) {
			ERRORL("Failed to create a vulkan descriptor pool\n");
		}

		VkDescriptorSetLayout* layouts = mam_stack_pusht(VkDescriptorSetLayout, mvk->stack, mvk->swap_chain_size);
		for_each_in(VkDescriptorSetLayout, layout, layouts, mvk->swap_chain_size) *layout = mvk->descriptor_set_layout;

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = mvk->descriptor_pool;
		alloc_info.descriptorSetCount = mvk->swap_chain_size;
		alloc_info.pSetLayouts = layouts;

		mvk->descriptor_sets = mam_stack_pusht(VkDescriptorSet, mvk->stack, mvk->swap_chain_size);
		if(vkAllocateDescriptorSets(mvk->device, &alloc_info, mvk->descriptor_sets) != VK_SUCCESS) {
			ERRORL("Failed to allocate vulkan descriptor sets\n");
		}

		for_each_lt(i, mvk->swap_chain_size) {
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = mvk->uniform_buffer;
			buffer_info.offset = i*sizeof(UniformBufferObject);
			buffer_info.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptor_write = {};
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = mvk->descriptor_sets[i];
			descriptor_write.dstBinding = 0;
			descriptor_write.dstArrayElement = 0;
			descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_write.descriptorCount = 1;
			descriptor_write.pBufferInfo = &buffer_info;
			descriptor_write.pImageInfo = 0; // Optional
			descriptor_write.pTexelBufferView = 0; // Optional

			vkUpdateDescriptorSets(mvk->device, 1, &descriptor_write, 0, 0);
		}
	}
}

void create_pipeline(MvkData* mvk) {
	/*
	VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
	VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
	VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them

	VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
	VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
	*/
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = mvk->surface_format.format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	if(vkCreateRenderPass(mvk->device, &render_pass_info, 0, &mvk->render_pass) != VK_SUCCESS) {
		ERRORL("Failed to create a vulkan render pass\n");
	}

	VkVertexInputBindingDescription vertex_description = {};
	#define VERTEX_ATTRIBUTES_SIZE 2
	VkVertexInputAttributeDescription vertex_attributes[VERTEX_ATTRIBUTES_SIZE];
	{//vertex info
		vertex_description.binding = 0;
		vertex_description.stride = sizeof(Vertex);
		vertex_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertex_attributes[0].binding = 0;
		vertex_attributes[0].location = 0;
		vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attributes[0].offset = offsetof(Vertex, pos);
		vertex_attributes[1].binding = 0;
		vertex_attributes[1].location = 1;
		vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attributes[1].offset = offsetof(Vertex, color);
	}

	//create pipeline
	VkPipelineVertexInputStateCreateInfo vertex_info = {};
	vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_info.vertexBindingDescriptionCount = 1;
	vertex_info.pVertexBindingDescriptions = &vertex_description; // Optional
	vertex_info.vertexAttributeDescriptionCount = VERTEX_ATTRIBUTES_SIZE;
	vertex_info.pVertexAttributeDescriptions = vertex_attributes; // Optional

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = cast(float, mvk->swap_chain_image_extent.width);
	viewport.height = cast(float, mvk->swap_chain_image_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = mvk->swap_chain_image_extent;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//controls which side of the face of a triangle is rendered, aka which side is see through
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = 0; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo color_blend = {};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend.logicOpEnable = VK_FALSE;
	color_blend.logicOp = VK_LOGIC_OP_COPY; // Optional
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &color_blend_attachment;
	color_blend.blendConstants[0] = 0.0f; // Optional
	color_blend.blendConstants[1] = 0.0f; // Optional
	color_blend.blendConstants[2] = 0.0f; // Optional
	color_blend.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = 1; // Optional
	layout_info.pSetLayouts = &mvk->descriptor_set_layout; // Optional
	layout_info.pushConstantRangeCount = 0; // Optional
	layout_info.pPushConstantRanges = 0; // Optional

	if(vkCreatePipelineLayout(mvk->device, &layout_info, 0, &mvk->pipeline_layout) != VK_SUCCESS) {
		ERRORL("Failed to create a vulkan pipeline layout\n");
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = mvk->shader_stages_size;
	pipeline_info.pStages = mvk->shader_stages;
	pipeline_info.pVertexInputState = &vertex_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = 0; // Optional
	pipeline_info.pColorBlendState = &color_blend;
	pipeline_info.pDynamicState = 0; // Optional
	pipeline_info.layout = mvk->pipeline_layout;
	pipeline_info.renderPass = mvk->render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipeline_info.basePipelineIndex = -1; // Optional

	if(vkCreateGraphicsPipelines(mvk->device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &mvk->pipeline) != VK_SUCCESS) {
		ERRORL("Failed to create a vulkan graphics pipeline\n");
	}
	{//create buffers
		mvk->frame_buffers = mam_stack_pusht(VkFramebuffer, mvk->stack, mvk->swap_chain_size);
		for_each_lt(i, mvk->swap_chain_size) {
			VkFramebufferCreateInfo frame_buffer_info = {};
			frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frame_buffer_info.renderPass = mvk->render_pass;
			frame_buffer_info.attachmentCount = 1;
			frame_buffer_info.pAttachments = &mvk->swap_chain_image_views[i];
			frame_buffer_info.width = mvk->swap_chain_image_extent.width;
			frame_buffer_info.height = mvk->swap_chain_image_extent.height;
			frame_buffer_info.layers = 1;

			if(vkCreateFramebuffer(mvk->device, &frame_buffer_info, 0, &mvk->frame_buffers[i]) != VK_SUCCESS) {
				ERRORL("Failed to create a vulkan frame buffer\n");
			}
		}

		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = mvk->command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = mvk->swap_chain_size;

		mvk->command_buffers = mam_stack_pusht(VkCommandBuffer, mvk->stack, mvk->swap_chain_size);
		if(vkAllocateCommandBuffers(mvk->device, &alloc_info, mvk->command_buffers) != VK_SUCCESS) {
			ERRORL("Failed to allocate vulkan command buffers");
		}

		for_each_lt(i, mvk->swap_chain_size) {
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = 0; // Optional
			begin_info.pInheritanceInfo = 0; // Optional

			if(vkBeginCommandBuffer(mvk->command_buffers[i], &begin_info) != VK_SUCCESS) {
				ERRORL("failed to begin recording command buffer!");
			}
			VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

			VkRenderPassBeginInfo render_begin_info = {};
			render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_begin_info.renderPass = mvk->render_pass;
			render_begin_info.framebuffer = mvk->frame_buffers[i];
			render_begin_info.renderArea.offset.x = 0;
			render_begin_info.renderArea.offset.y = 0;
			render_begin_info.renderArea.extent = mvk->swap_chain_image_extent;
			render_begin_info.clearValueCount = 1;
			render_begin_info.pClearValues = &clear_color;

			vkCmdBeginRenderPass(mvk->command_buffers[i], &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);


			vkCmdBindPipeline(mvk->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mvk->pipeline);
			VkDeviceSize offsets = 0;
			int32 indices_size = INDEX_BUFFER_SIZE/sizeof(int32);

			vkCmdBindDescriptorSets(mvk->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mvk->pipeline_layout, 0, 1, &mvk->descriptor_sets[i], 0, 0);
			vkCmdBindVertexBuffers(mvk->command_buffers[i], 0, 1, &mvk->vertex_buffer, &offsets);
			vkCmdBindIndexBuffer(mvk->command_buffers[i], mvk->index_buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(mvk->command_buffers[i], indices_size, 1, 0, 0, 0);

			vkCmdEndRenderPass(mvk->command_buffers[i]);
			if(vkEndCommandBuffer(mvk->command_buffers[i]) != VK_SUCCESS) {
				ERRORL("Failed to record to the vulkan command buffer");
			}
		}
	}
}

void recreate_swap_chain(MvkData* mvk, SDL_Window* window) {
	vkDeviceWaitIdle(mvk->device);
	{//clean up old swapchain
		for_each_in(VkFramebuffer, frame_buffer, mvk->frame_buffers, mvk->swap_chain_size) vkDestroyFramebuffer(mvk->device, *frame_buffer, 0);

		vkFreeCommandBuffers(mvk->device, mvk->command_pool, mvk->swap_chain_size, mvk->command_buffers);

		vkDestroyPipeline(mvk->device, mvk->pipeline, 0);
		vkDestroyPipelineLayout(mvk->device, mvk->pipeline_layout, 0);
		vkDestroyRenderPass(mvk->device, mvk->render_pass, 0);
		for_each_in(VkImageView, image_view, mvk->swap_chain_image_views, mvk->swap_chain_size) vkDestroyImageView(mvk->device, *image_view, 0);
		vkDestroySwapchainKHR(mvk->device, mvk->swap_chain, 0);

		vkDestroyBuffer(mvk->device, mvk->uniform_buffer, 0);
		vkFreeMemory(mvk->device, mvk->uniform_buffer_memory, 0);
		vkDestroyDescriptorPool(mvk->device, mvk->descriptor_pool, 0);
		mam_stack_set_size(mvk->stack, mvk->swap_chain_mem_start);
	}
	find_device_capabilities(mvk, window);
	create_swap_chain(mvk);
	create_pipeline(mvk);
}





void game_free_recursively(GameMemDesc* desc) {
	for_each_lt(i, desc->children_total) {
		GameMemDesc* child = &cast(GameMemDesc*, desc->mem)[i];

		if(!(child->flags & GAME_MEMDESC_INTERNAL)) {
			game_free_recursively(child);
		}
	}
	free(desc->mem);
	desc->mem = 0;
}
//NOTE: all memory for the game's internals should be allocated through this function
GameMemDesc alloc_game_mem(inta alloc_size, int children_total, uint flags) {
	GameMemDesc desc;
	desc.mem = malloc(alloc_size);
	desc.alloc_size = alloc_size;
	desc.children_total = children_total;
	desc.flags = flags;
	#ifdef DEBUG
		memset(desc.mem, 'a', alloc_size);
	#endif
	return desc;
}

GameMemDesc game_new() {
	GameMemDesc game_mem_desc = alloc_game_mem(GAME_STACK_SIZE, 2, 0);

	Game* game = (Game*)game_mem_desc.mem;
	memzero(game, 1);

	game->stack_desc.mem = ptr_add(void, game, sizeof(Game));
	game->stack_desc.alloc_size = GAME_STACK_SIZE - sizeof(Game);
	game->stack_desc.children_total = 0;
	game->stack_desc.flags = GAME_MEMDESC_INTERNAL;
	mam_stack_init(game->stack_desc.mem, game->stack_desc.alloc_size);

	game->temp_stack_desc = alloc_game_mem(TEMP_STACK_SIZE, 0, GAME_MEMDESC_TEMP);
	mam_stack_init(game->temp_stack_desc.mem, game->temp_stack_desc.alloc_size);

	//TODO: find a better rng seed
	pcg_seed(&game->rng, 12);

	game->lifetime = 0.0;
	game->do_draw = 1;

	{//init game state
		game->grid_w = 4;
		game->grid_h = 4;
		game->grid = mam_stack_pusht(int32, game->stack, game->grid_h*game->grid_w);
		memzero(game->grid, game->grid_h*game->grid_w);
		int32 v = pcg_random_in(&game->rng, 1, 2);
		int32 x = pcg_random_in(&game->rng, 0, game->grid_w);
		int32 y = pcg_random_in(&game->rng, 0, game->grid_h);
		game->grid[x + game->grid_w*y] = v;
	}

	return game_mem_desc;
}

Output game_update(Game* game, double delta) {
	Output output = {};


	{//clear transient data
		mam_stack_set_size(game->temp_stack, 0);
		game->input_left_just_down = 0;
		game->input_right_just_down = 0;
		game->input_up_just_down = 0;
		game->input_down_just_down = 0;
	}

	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			output.game_quit = 1;
			break;
		} else if(event.type == SDL_TEXTINPUT) {
		} else if(event.type == SDL_TEXTEDITING) {
		} else if(event.type == SDL_MOUSEMOTION) {
		} else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
			if(event.button.button == SDL_BUTTON_LEFT) {
			} else if(event.button.button == SDL_BUTTON_RIGHT) {
			} else if(event.button.button == SDL_BUTTON_MIDDLE) {
			}
		} else if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
			auto scancode = event.key.keysym.scancode;
			auto keycode = event.key.keysym.sym;
			bool is_down = (event.key.state == SDL_PRESSED);
			bool is_repeat = (event.key.repeat > 0);
			//TODO: Support full suite of keys
			if(is_down) {
				if(keycode == SDLK_ESCAPE) {
					output.game_quit = 1;
					break;
					// } else if(keycode == SDLK_BACKSPACE) {
					// 	input_add_char(&input, CHAR_BACKSPACE, startup_stack);
					// } else if(keycode == SDLK_DELETE) {
					// 	input_add_char(&input, CHAR_DELETE, startup_stack);
					// } else if(keycode == SDLK_TAB) {
					// 	input_add_char(&input, CHAR_TAB, startup_stack);
					// } else if(keycode == SDLK_RETURN) {
					// 	//NOTE: there's a SDLK_RETURN2??
					// 	input_add_char(&input, CHAR_RETURN, startup_stack);
					// } else if(keycode == SDLK_DOWN) {
					// 	input_add_char(&input, CHAR_DOWN, startup_stack);
					// } else if(keycode == SDLK_UP) {
					// 	input_add_char(&input, CHAR_UP, startup_stack);
					// } else if(keycode == SDLK_RIGHT) {
					// 	input_add_char(&input, CHAR_RIGHT, startup_stack);
					// } else if(keycode == SDLK_LEFT) {
					// 	//NOTE: there's a SDLK_RETURN2??
					// 	input_add_char(&input, CHAR_LEFT, startup_stack);
				} else {
				}
			} else {
			}
			if(!is_repeat) {
				if(keycode == SDLK_LEFT) {
					game->input_left_just_down |= is_down & !game->input_left_down;
					game->input_left_down = is_down;
				} else if(keycode == SDLK_RIGHT) {
					game->input_right_just_down |= is_down & !game->input_right_down;
					game->input_right_down = is_down;
				} else if(keycode == SDLK_UP) {
					game->input_up_just_down |= is_down & !game->input_up_down;
					game->input_up_down = is_down;
				} else if(keycode == SDLK_DOWN) {
					game->input_down_just_down |= is_down & !game->input_down_down;
					game->input_down_down = is_down;
				} else if(keycode == SDLK_LSHIFT) {
				} else if(keycode == SDLK_LCTRL) {
				} else if(keycode == SDLK_0) {
				} else if(keycode == SDLK_1) {
				} else if(keycode == SDLK_2) {
				} else if(keycode == SDLK_3) {
				} else if(keycode == SDLK_4) {
				} else if(keycode == SDLK_5) {
				} else if(keycode == SDLK_6) {
				} else if(keycode == SDLK_7) {
				} else if(keycode == SDLK_8) {
				} else if(keycode == SDLK_9) {
				}
			}
		} else if (event.type == SDL_WINDOWEVENT) {
			auto window_id = event.window.event;
			if (window_id == SDL_WINDOWEVENT_CLOSE) {
				output.game_quit = 1;
				break;
			} else if(window_id == SDL_WINDOWEVENT_SIZE_CHANGED) {
				output.window_resize = 1;
			} else if (window_id == SDL_WINDOWEVENT_SHOWN) {
				game->do_draw = 1;
			} else if (window_id == SDL_WINDOWEVENT_HIDDEN) {
				game->do_draw = 0;
			} else if (window_id == SDL_WINDOWEVENT_MINIMIZED) {
				game->do_draw = 0;
			} else if (window_id == SDL_WINDOWEVENT_MAXIMIZED) {
			} else if (window_id == SDL_WINDOWEVENT_EXPOSED) {
				game->do_draw = 1;
			} else if (window_id == SDL_WINDOWEVENT_ENTER) {
			} else if (window_id == SDL_WINDOWEVENT_LEAVE) {
			} else if (window_id == SDL_WINDOWEVENT_FOCUS_GAINED) {
			} else if (window_id == SDL_WINDOWEVENT_FOCUS_LOST) {
			}
		}
	}
	if(output.game_quit) return output;

	{//update game
		bool moved = 0;
		if(game->input_left_just_down) {
			for_each_in_range(x, 1, game->grid_w - 1) {
				for_each_lt(y, game->grid_h) {
					int32* v = &game->grid[x + game->grid_w*y];
					if(*v == 0) continue;
					for_each_in_range_bw(coll_x, 0, x - 1) {
						int32* coll_v = &game->grid[coll_x + game->grid_w*y];
						if(*coll_v == 0) {
							*coll_v = *v;
							*v = 0;
							v = coll_v;
							moved = 1;
						} else if(*coll_v == *v) {
							*coll_v += 1;
							*v = 0;
							moved = 1;
							break;
						} else {
							break;
						}
					}
				}
			}
		} else if(game->input_right_just_down) {
			for_each_in_range_bw(x, 0, game->grid_w - 2) {
				for_each_lt(y, game->grid_h) {
					int32* v = &game->grid[x + game->grid_w*y];
					if(*v == 0) continue;
					for_each_in_range(coll_x, x + 1, game->grid_w - 1) {
						int32* coll_v = &game->grid[coll_x + game->grid_w*y];
						if(*coll_v == 0) {
							*coll_v = *v;
							*v = 0;
							v = coll_v;
							moved = 1;
						} else if(*coll_v == *v) {
							*coll_v += 1;
							*v = 0;
							moved = 1;
							break;
						} else {
							break;
						}
					}
				}
			}
		} else if(game->input_up_just_down) {
			for_each_in_range(y, 0, game->grid_h - 1) {
				for_each_lt(x, game->grid_w) {
					int32* v = &game->grid[x + game->grid_w*y];
					if(*v == 0) continue;
					for_each_in_range_bw(coll_y, 0, y - 1) {
						int32* coll_v = &game->grid[x + game->grid_w*coll_y];
						if(*coll_v == 0) {
							*coll_v = *v;
							*v = 0;
							v = coll_v;
							moved = 1;
						} else if(*coll_v == *v) {
							*coll_v += 1;
							*v = 0;
							moved = 1;
							break;
						} else {
							break;
						}
					}
				}
			}
		} else if(game->input_down_just_down) {
			for_each_in_range_bw(y, 0, game->grid_h - 2) {
				for_each_lt(x, game->grid_w) {
					int32* v = &game->grid[x + game->grid_w*y];
					if(*v == 0) continue;
					for_each_in_range(coll_y, y + 1, game->grid_h - 1) {
						int32* coll_v = &game->grid[x + game->grid_w*coll_y];
						if(*coll_v == 0) {
							*coll_v = *v;
							*v = 0;
							v = coll_v;
							moved = 1;
						} else if(*coll_v == *v) {
							*coll_v += 1;
							*v = 0;
							moved = 1;
							break;
						} else {
							break;
						}
					}
				}
			}
		}
		if(moved) {
			int32** empty_cells = mam_stack_pusht(int32*, game->temp_stack, game->grid_h*game->grid_w);
			int32 empty_cells_size = 0;
			for_each_lt(y, game->grid_h) {
				for_each_lt(x, game->grid_w) {
					int32* v = &game->grid[x + game->grid_w*y];
					if(*v == 0) {
						empty_cells[empty_cells_size] = v;
						empty_cells_size += 1;
					}
				}
			}
			if(empty_cells_size > 0) {
				*empty_cells[pcg_random_in(&game->rng, 0, empty_cells_size - 1)] = pcg_random_in(&game->rng, 1, 2);
			}
		}
		// game
	}


	game->lifetime += delta;

	output.do_draw = game->do_draw;
	return output;
}


void game_render(Game* game, double delta, MvkData* mvk, uint32 image_i) {

	int32 vbuffer_size = VERTEX_BUFFER_SIZE;
	VkBuffer staging_vbuffer;
	VkDeviceMemory staging_vbuffer_memory;
	create_buffer(mvk, vbuffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_vbuffer, &staging_vbuffer_memory);

	int32 ibuffer_size = INDEX_BUFFER_SIZE;
	VkBuffer staging_ibuffer;
	VkDeviceMemory staging_ibuffer_memory;
	create_buffer(mvk, ibuffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_ibuffer, &staging_ibuffer_memory);

	//transfer to vertex buffer
	byte* vbuffer;
	byte* ibuffer;
	byte* ubuffer;
	int32 vbuffer_i = 0;
	int32 ibuffer_i = 0;
	int32 ubuffer_i = 0;
	vkMapMemory(mvk->device, staging_vbuffer_memory, 0, vbuffer_size, 0, (void**)&vbuffer);
	vkMapMemory(mvk->device, staging_ibuffer_memory, 0, ibuffer_size, 0, (void**)&ibuffer);
	vkMapMemory(mvk->device, mvk->uniform_buffer_memory, image_i*sizeof(UniformBufferObject), sizeof(UniformBufferObject), 0, (void**)&ubuffer);

	{//fill gpu buffers
		static const int32 colors_size = 12;
		gbVec3 colors[colors_size] = {
			{1.0f, 1.0f, 1.0f},
		};
		for_each_in_range(i, 1, colors_size - 1) {
			float t = (i - 1.0f)/(colors_size - 2.0f);
			if(t <= .5) {
				t *= 2;
				colors[i].b = 1.0f - t;
				colors[i].r = t;
			} else {
				t = 2*t - 1;
				colors[i].r = 1.0f - t;
				colors[i].g = t;
			}
		}

		float screen_w = mvk->swap_chain_image_extent.width;
		float screen_h = mvk->swap_chain_image_extent.height;
		float pixel_l = min(screen_w, screen_h);

		// int32 grid_x = 10;
		// int32 grid_y = 10;
		// int32 grid_w = pixel_l - 10;
		// int32 grid_h = pixel_l - 10;
		float square_base_l = gb_floor(pixel_l/4);
		float square_l = square_base_l - 20;
		for_each_lt(y, game->grid_h) {
			for_each_lt(x, game->grid_w) {
				int32 v = game->grid[x + game->grid_w*y];
				float square_x = square_base_l*x + 10;
				float square_y = square_base_l*y + 10;
				gbVec3 color = colors[min(colors_size - 1, v)];
				Vertex square[4] = {
					{{square_x, square_y}, color},
					{{square_x + square_l, square_y}, color},
					{{square_x + square_l, square_y + square_l}, color},
					{{square_x, square_y + square_l}, color}
				};
				int32 base_i = vbuffer_i/sizeof(Vertex);
				int32 square_is[6] = {
					base_i, base_i + 1, base_i + 2, base_i + 2, base_i + 3, base_i
				};
				memcpy(vbuffer + vbuffer_i, square, 4*sizeof(Vertex));
				vbuffer_i += 4*sizeof(Vertex);
				memcpy(ibuffer + ibuffer_i, square_is, 6*sizeof(int32));
				ibuffer_i += 6*sizeof(int32);
			}
		}


		UniformBufferObject ubo;
		gb_mat4_identity(&ubo.model);
		if(screen_w >= screen_h) {
			ubo.model.w.x += (screen_w - screen_h)/2.0f;
		} else {
			ubo.model.w.y += (screen_h - screen_w)/2.0f;
		}
		ubo.model.x.x *= 2.0f/screen_w;
		ubo.model.w.x *= 2.0f/screen_w;
		ubo.model.y.y *= 2.0f/screen_h;
		ubo.model.w.y *= 2.0f/screen_h;
		ubo.model.w.x += -1.0f;
		ubo.model.w.y += -1.0f;

		memcpy(ubuffer, &ubo, sizeof(UniformBufferObject));
		ubuffer_i += sizeof(UniformBufferObject);
	}

	vkUnmapMemory(mvk->device, staging_vbuffer_memory);
	vkUnmapMemory(mvk->device, staging_ibuffer_memory);
	vkUnmapMemory(mvk->device, mvk->uniform_buffer_memory);
	copy_buffer(mvk, mvk->vertex_buffer, staging_vbuffer, vbuffer_i);
	copy_buffer(mvk, mvk->index_buffer, staging_ibuffer, ibuffer_i);
	vkDestroyBuffer(mvk->device, staging_vbuffer, 0);
	vkDestroyBuffer(mvk->device, staging_ibuffer, 0);
	vkFreeMemory(mvk->device, staging_vbuffer_memory, 0);
	vkFreeMemory(mvk->device, staging_ibuffer_memory, 0);
}



int main() {
	//there are only 2 exit points for this program, the return from the bottom of main and main_trap
	MainTrash trash = {};
	mam_set_error_trap(main_trap, &trash);

	gbVec2 window_dim = gb_vec2(1200, 800);
	SDL_Window* window = 0;
	double time_per_frame = DEFAULT_FPS;

	MvkData mvk_mem = {};
	MvkData* mvk = &mvk_mem;
	trash.mvk = mvk;

	mvk->stack = mam_stack_init(malloc(MEGABYTE), MEGABYTE);
	trash.ptrs[0] = mvk->stack;
	{//init
		uint32 sdlvk_extensions_size;
		const char** sdlvk_extensions;
		{//init SDL
			if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
				char str[512] = {};
				snprintf(str, 512, "Could not initialize SDL: %s\n", SDL_GetError());
				MAM_ERRORL(str);
			}
			trash.sdl_isinit = 1;

			uint32 window_options = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
			window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_dim.x, window_dim.y, window_options);

			if(!window) {
				char str[512] = {};
				snprintf(str, 512, "Could not create window: %s\n", SDL_GetError());
				MAM_ERRORL(str);
			}
			trash.window = window;

			int display_index = SDL_GetWindowDisplayIndex(window);
			SDL_DisplayMode dm;
			//TODO: handle change in monitor
			if(SDL_GetDesktopDisplayMode(display_index, &dm) < 0) {
				printf("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
			} else {
				time_per_frame = 1.0/dm.refresh_rate;
			}

			SDL_Vulkan_GetInstanceExtensions(window, &sdlvk_extensions_size, 0);
			sdlvk_extensions = mam_stack_pusht(const char*, mvk->stack, sdlvk_extensions_size);
			SDL_Vulkan_GetInstanceExtensions(window, &sdlvk_extensions_size, sdlvk_extensions);
		}
		uint32 mvk_desired_layers_size = 0;
		char** mvk_desired_layers = 0;
		{//instance and surface creation
			uint32 mvk_extensions_size = 0;
			vkEnumerateInstanceExtensionProperties(0, &mvk_extensions_size, 0);
			VkExtensionProperties* mvk_extensions = mam_stack_pusht(VkExtensionProperties, mvk->stack, mvk_extensions_size);
			vkEnumerateInstanceExtensionProperties(0, &mvk_extensions_size, mvk_extensions);

			uint32 mvk_layers_size = 0;
			vkEnumerateInstanceLayerProperties(&mvk_layers_size, 0);
			VkLayerProperties* mvk_layers = mam_stack_pusht(VkLayerProperties, mvk->stack, mvk_layers_size);
			vkEnumerateInstanceLayerProperties(&mvk_layers_size, mvk_layers);

			mvk_desired_layers = mam_stack_pusht(char*, mvk->stack, 0);
			#ifdef DEBUG
			for_each_in(char*, desired_debug_layer, MVK_DEBUG_LAYERS, MVK_DEBUG_LAYERS_SIZE) {
				int flag = 1;
				for_each_in(VkLayerProperties, layer, mvk_layers, mvk_layers_size) {
					if(mam_streq(mam_tostr(*desired_debug_layer), mam_tostr(layer->layerName))) {
						flag = 0;
						mvk_desired_layers_size += 1;
						mam_stack_extend(mvk->stack, mvk_desired_layers, sizeof(*mvk_desired_layers)*mvk_desired_layers_size);
						mvk_desired_layers[mvk_desired_layers_size - 1] = *desired_debug_layer;
					}
				}
				if(flag) {
					char str[512] = {};
					snprintf(str, 512, "Could not find the desired vulkan layer: %s\n", *desired_debug_layer);
					MAM_ERRORL(str);
				}
			}
			#endif

			VkApplicationInfo mvk_app_info = {};
			mvk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			mvk_app_info.pApplicationName = WINDOW_NAME;
			mvk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			mvk_app_info.pEngineName = MVK_ENGINE_NAME;
			mvk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			mvk_app_info.apiVersion = VK_API_VERSION_1_0;

			VkInstanceCreateInfo mvk_info = {};
			mvk_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			mvk_info.pApplicationInfo = &mvk_app_info;
			mvk_info.enabledExtensionCount = sdlvk_extensions_size;
			mvk_info.ppEnabledExtensionNames = sdlvk_extensions;
			mvk_info.enabledLayerCount = mvk_desired_layers_size;
			mvk_info.ppEnabledLayerNames = mvk_desired_layers;

			if(vkCreateInstance(&mvk_info, 0, &mvk->instance) != VK_SUCCESS) {
				MAM_ERRORL("Could not create a vulkan instance\n");
			}

			if(SDL_Vulkan_CreateSurface(window, mvk->instance, &mvk->surface) != SDL_TRUE) {
				MAM_ERRORL("Failed to create a vulkan surface\n");
			}
		}
		{//pick physical device
			mvk->physical_device = VK_NULL_HANDLE;
			uint32 mvk_devices_size = 0;
			vkEnumeratePhysicalDevices(mvk->instance, &mvk_devices_size, 0);
			VkPhysicalDevice* mvk_devices = mam_stack_pusht(VkPhysicalDevice, mvk->stack, mvk_devices_size);
			vkEnumeratePhysicalDevices(mvk->instance, &mvk_devices_size, mvk_devices);

			int highest_rating = 0;
			for_each_index(VkPhysicalDevice, i, device, mvk_devices, mvk_devices_size) {
				inta mvk_stack_size = mvk->stack->size;

				VkPhysicalDeviceProperties properties = {};
				VkPhysicalDeviceFeatures features = {};
				vkGetPhysicalDeviceProperties(*device, &properties);
				vkGetPhysicalDeviceFeatures(*device, &features);

				uint32_t device_extensions_size = 0;
				vkEnumerateDeviceExtensionProperties(*device, 0, &device_extensions_size, 0);
				VkExtensionProperties* device_extensions = mam_stack_pusht(VkExtensionProperties, mvk->stack, device_extensions_size);
				vkEnumerateDeviceExtensionProperties(*device, 0, &device_extensions_size, device_extensions);

				uint32 mvk_queues_size = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(*device, &mvk_queues_size, 0);
				VkQueueFamilyProperties* mvk_queues = mam_stack_pusht(VkQueueFamilyProperties, mvk->stack, mvk_queues_size);
				vkGetPhysicalDeviceQueueFamilyProperties(*device, &mvk_queues_size, mvk_queues);

				int rating = 1;
				// discrete GPUs have a significant performance advantage
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					rating += 1024;
				}
				// maximum possible size of textures affects graphics quality
				rating += properties.limits.maxImageDimension2D;
				// game can't function without geometry shaders
				rating *= features.geometryShader != 0;
				bool has_required_extensions = 0;
				for_each_in(char*, desired_extension, MVK_DEVICE_EXTENSIONS, MVK_DEVICE_EXTENSIONS_SIZE) {
					for_each_in(VkExtensionProperties, extension, device_extensions, device_extensions_size) {
						if(mam_streq(mam_tostr(*desired_extension), mam_tostr(extension->extensionName))) {
							has_required_extensions = 1;
							break;
						}
					}
				}
				// check for extensions
				rating *= has_required_extensions;
				if(!rating) {
					mam_stack_set_size(mvk->stack, mvk_stack_size);
					continue;
				}

				int32 best_draw_queue_i = -1;
				int32 best_present_queue_i = -1;
				//"It is important that we only try to query for swap chain support after verifying that the extension is available."
				uint32 formats_size = 0;
				vkGetPhysicalDeviceSurfaceFormatsKHR(*device, mvk->surface, &formats_size, 0);

				uint32 present_modes_size = 0;
				vkGetPhysicalDeviceSurfacePresentModesKHR(*device, mvk->surface, &present_modes_size, 0);
				rating *= present_modes_size > 0 && formats_size > 0;
				if(!rating) {
					mam_stack_set_size(mvk->stack, mvk_stack_size);
					continue;
				}


				for_each_index_bw(VkQueueFamilyProperties, j, queue, mvk_queues, mvk_queues_size) {
					VkBool32 can_present = 0;
					vkGetPhysicalDeviceSurfaceSupportKHR(*device, j, mvk->surface, &can_present);
					if(queue->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						best_draw_queue_i = j;
						if(can_present) {
							best_present_queue_i = j;
							// device is faster if it only uses 1 queue
							rating += 1023;
							break;
						}
					} else if(can_present) {
						best_present_queue_i = j;
					}
				}
				// device must support drawing and presenting
				rating *= best_draw_queue_i >= 0 && best_present_queue_i >= 0;
				if(!rating) {
					mam_stack_set_size(mvk->stack, mvk_stack_size);
					continue;
				}

				if(highest_rating < rating) {
					highest_rating = rating;
					mvk->physical_device = *device;
					mvk->draw_queue_i = best_draw_queue_i;
					mvk->present_queue_i = best_present_queue_i;
				}
				mam_stack_set_size(mvk->stack, mvk_stack_size);
			}
			if(mvk->physical_device == VK_NULL_HANDLE) {
				MAM_ERRORL("Could not find an adequate vulkan compatible gpu\n");
			}
		}
		{//create logical device and record features
			float queue_priority = 1.0f;
			VkDeviceQueueCreateInfo mvk_queue_infos[2] = {{}};
			mvk_queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			mvk_queue_infos[0].queueFamilyIndex = mvk->draw_queue_i;
			mvk_queue_infos[0].queueCount = 1;
			mvk_queue_infos[0].pQueuePriorities = &queue_priority;
			if(mvk->draw_queue_i != mvk->present_queue_i) {
				mvk_queue_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				mvk_queue_infos[1].queueFamilyIndex = mvk->present_queue_i;
				mvk_queue_infos[1].queueCount = 1;
				mvk_queue_infos[1].pQueuePriorities = &queue_priority;
			}

			VkPhysicalDeviceFeatures mvk_device_features = {};

			VkDeviceCreateInfo mvk_device_info = {};
			mvk_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			mvk_device_info.pQueueCreateInfos = mvk_queue_infos;
			mvk_device_info.queueCreateInfoCount = (mvk->draw_queue_i == mvk->present_queue_i) ? 1 : 2;
			mvk_device_info.pEnabledFeatures = &mvk_device_features;
			mvk_device_info.enabledLayerCount = mvk_desired_layers_size;
			mvk_device_info.ppEnabledLayerNames = mvk_desired_layers;
			mvk_device_info.enabledExtensionCount = MVK_DEVICE_EXTENSIONS_SIZE;
			mvk_device_info.ppEnabledExtensionNames = MVK_DEVICE_EXTENSIONS;

			if(vkCreateDevice(mvk->physical_device, &mvk_device_info, 0, &mvk->device) != VK_SUCCESS) {
				MAM_ERRORL("Failed to create a vulkan logical device\n");
			}

			vkGetDeviceQueue(mvk->device, mvk->draw_queue_i, 0, &mvk->draw_queue);
			vkGetDeviceQueue(mvk->device, mvk->present_queue_i, 0, &mvk->present_queue);
		}
		{//create semaphores and fences
			VkSemaphore* sems = mam_stack_pusht(VkSemaphore, mvk->stack, 2*MVK_FRAMES_IN_FLIGHT);
			VkSemaphoreCreateInfo semaphore_info = {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			for_each_lt(i, 2*MVK_FRAMES_IN_FLIGHT) {
				if(vkCreateSemaphore(mvk->device, &semaphore_info, 0, &sems[i]) != VK_SUCCESS) {
					ERRORL("Failed to create a vulkan semaphore\n");
				}
			}
			mvk->image_available_sems = sems;
			mvk->render_finished_sems = &sems[MVK_FRAMES_IN_FLIGHT];

			mvk->in_flight_fences = mam_stack_pusht(VkFence, mvk->stack, MVK_FRAMES_IN_FLIGHT);
			mvk->images_in_flight_fences = mam_stack_pusht(VkFence, mvk->stack, MVK_FRAMES_IN_FLIGHT);
			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for_each_lt(i, MVK_FRAMES_IN_FLIGHT) {
				if(vkCreateFence(mvk->device, &fence_info, 0, &mvk->in_flight_fences[i]) != VK_SUCCESS) {
					ERRORL("Failed to create a vulkan fence\n");
				}
				mvk->images_in_flight_fences[i] = VK_NULL_HANDLE;
			}
		}
		{//create shaders
			VkShaderModule shader_frag = {};
			VkShaderModule shader_vert = {};
			VkShaderModuleCreateInfo shader_info = {};

			MamString frag_code = read_file_to_stack(MVK_SHADER_FRAG, mvk->stack);
			shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_info.codeSize = frag_code.size;
			shader_info.pCode = (uint32*)frag_code.ptr;
			if(vkCreateShaderModule(mvk->device, &shader_info, 0, &shader_frag) != VK_SUCCESS) {
				MAM_ERRORL("Failed to create the vulkan fragment shader\n");
			}
			MamString vert_code = read_file_to_stack(MVK_SHADER_VERT, mvk->stack);
			memzero(&shader_info, 1);
			shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_info.codeSize = vert_code.size;
			shader_info.pCode = (uint32*)vert_code.ptr;
			if(vkCreateShaderModule(mvk->device, &shader_info, 0, &shader_vert) != VK_SUCCESS) {
				MAM_ERRORL("Failed to create the vulkan fragment shader\n");
			}

			VkPipelineShaderStageCreateInfo shader_frag_info = {};
			shader_frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_frag_info.module = shader_frag;
			shader_frag_info.pName = "main";
			shader_frag_info.pSpecializationInfo = 0;//can use this for compile time constants

			VkPipelineShaderStageCreateInfo shader_vert_info = {};
			shader_vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shader_vert_info.module = shader_vert;
			shader_vert_info.pName = "main";
			shader_vert_info.pSpecializationInfo = 0;//can use this for compile time constants

			mvk->shader_stages = mam_stack_pusht(VkPipelineShaderStageCreateInfo, mvk->stack, 2);
			mvk->shader_stages[0] = shader_vert_info;
			mvk->shader_stages[1] = shader_frag_info;
			mvk->shader_stages_size = 2;
		}
		{//create command pool
			VkCommandPoolCreateInfo command_pool_info = {};
			command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			command_pool_info.queueFamilyIndex = mvk->draw_queue_i;
			command_pool_info.flags = 0; // Optional
			if(vkCreateCommandPool(mvk->device, &command_pool_info, 0, &mvk->command_pool) != VK_SUCCESS) {
				ERRORL("Failed to create a vulkan command pool\n");
			}
		}
		{//create vertex buffer
			mvk->vertex_buffer_size = VERTEX_BUFFER_SIZE;

			create_buffer(mvk, sizeof(Vertex)*mvk->vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mvk->vertex_buffer, &mvk->vertex_buffer_memory);
		}
		{//create index buffer
			mvk->index_buffer_size = INDEX_BUFFER_SIZE;

			create_buffer(mvk, sizeof(int32)*mvk->index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mvk->index_buffer, &mvk->index_buffer_memory);
		}
		{//create descriptor set layout
			VkDescriptorSetLayoutBinding ubo_layout_binding = {};
			ubo_layout_binding.binding = 0;
			ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			ubo_layout_binding.descriptorCount = 1;
			ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			ubo_layout_binding.pImmutableSamplers = 0;

			VkDescriptorSetLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layout_info.bindingCount = 1;
			layout_info.pBindings = &ubo_layout_binding;

			if(vkCreateDescriptorSetLayout(mvk->device, &layout_info, 0, &mvk->descriptor_set_layout) != VK_SUCCESS) {
				ERRORL("Failed to create a vulkan descriptor set layout");
			}
		}
		find_device_capabilities(mvk, window);
		create_swap_chain(mvk);
		create_pipeline(mvk);
	}

	int64 counts_per_frame = cast(int64, gb_floor(time_per_frame*SDL_GetPerformanceFrequency()));

	uint64 frame_boundary = SDL_GetPerformanceCounter();
	double frame_duration = 0;
	int64 lifetime_frames = 0;
	double lifetime = 0;
	int64 dropped_frames = 0;

	trash.game_desc = game_new();
	Game* game = (Game*)trash.game_desc.mem;

	while(1) {
		//update game
		double delta = min(frame_duration, MAX_UPDATE_DELTA);
		Output output = game_update(game, delta);

		if(output.game_quit) break;
		if(output.window_resize) recreate_swap_chain(mvk, window);
		if(output.do_draw) {//draw then present frame
			int32 frame_i = lifetime_frames%MVK_FRAMES_IN_FLIGHT;
			uint32 image_i = 0;

			vkWaitForFences(mvk->device, 1, &mvk->in_flight_fences[frame_i], VK_TRUE, UINT64_MAX);

			VkResult result = vkAcquireNextImageKHR(mvk->device, mvk->swap_chain, MAX_UINT64, mvk->image_available_sems[frame_i], VK_NULL_HANDLE, &image_i);
			if(result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				ERRORL("Failed to acquire a vulkan swap chain image");
			}

			// Check if a previous frame is using this image (i.e. there is its fence to wait on)
			if(mvk->images_in_flight_fences[image_i] != VK_NULL_HANDLE) {
				vkWaitForFences(mvk->device, 1, &mvk->images_in_flight_fences[image_i], VK_TRUE, UINT64_MAX);
			}
			// Mark the image as now being in use by this frame
			mvk->images_in_flight_fences[image_i] = mvk->in_flight_fences[frame_i];

			vkResetFences(mvk->device, 1, &mvk->in_flight_fences[frame_i]);


			//render the frame
			game_render(game, delta, mvk, image_i);




			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo submit_info = {};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &mvk->image_available_sems[frame_i];
			submit_info.pWaitDstStageMask = &wait_stage;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &mvk->command_buffers[image_i];
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &mvk->render_finished_sems[frame_i];
			if(vkQueueSubmit(mvk->draw_queue, 1, &submit_info, mvk->in_flight_fences[frame_i]) != VK_SUCCESS) {
				ERRORL("Failed to submit to a vulkan queue\n");
			}

			VkPresentInfoKHR present_info = {};
			present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present_info.waitSemaphoreCount = 1;
			present_info.pWaitSemaphores = &mvk->render_finished_sems[frame_i];
			present_info.swapchainCount = 1;
			present_info.pSwapchains = &mvk->swap_chain;
			present_info.pImageIndices = &image_i;
			present_info.pResults = 0; // Optional
			result = vkQueuePresentKHR(mvk->present_queue, &present_info);
			if(result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR && result != VK_SUCCESS) {
				ERRORL("Failed to present a vulkan swap chain image");
			}
		}

		{//control framerate
			uint64 compute_boundary = SDL_GetPerformanceCounter();
			double time_to_compute = get_delta_time(frame_boundary, compute_boundary);
			uint64 new_frame_boundary = compute_boundary;

			#if FPS_PRINTOUT_FREQUENCY > 1
			if(lifetime_frames%FPS_PRINTOUT_FREQUENCY == 1) {
				// printf("compute time: %2.2fHz\n", 1/time_to_compute);
				printf("frame duration: %2.2fHz\n", 1/frame_duration);
				// printf("dropped frames: %d\n", dropped_frames);
			}
			#endif

			#if !PEDAL_TO_THE_METAL
			if(!mvk->device_does_vsync || !output.do_draw) {//vsync does not work when nothing is drawing
				if(time_to_compute < time_per_frame) {
					double time_to_wait = time_per_frame - time_to_compute - DELAY_RESOLUTION;
					if(time_to_wait > 0) {
						SDL_Delay(cast(uint32, 1000.0*time_to_wait));
					}
					new_frame_boundary = SDL_GetPerformanceCounter();
					while((new_frame_boundary - frame_boundary) < counts_per_frame) {
						new_frame_boundary = SDL_GetPerformanceCounter();
					}
				} else {
					dropped_frames += 1;
				}
			}
			#endif
			frame_duration = get_delta_time(frame_boundary, new_frame_boundary);
			frame_boundary = new_frame_boundary;
			lifetime += frame_duration;
			lifetime_frames += 1;
		}
	}

	main_cleanup(&trash);
	return 0;
}
