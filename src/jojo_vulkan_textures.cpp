#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/*
	Resources to deal with:
	* Staging Buffer
	* Staging Memory
	* Image
	* Device Memory
	* Sampler
	* ImageView
*/

static void setImageLayout (
	VkCommandBuffer cmd,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkImageSubresourceRange range
) {
	VkImageMemoryBarrier b = {};
	b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	b.oldLayout = oldLayout;
	b.newLayout = newLayout;
	b.image = image;
	b.subresourceRange = range;

	switch (oldLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		b.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		b.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	}

	switch (newLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	}

	VkPipelineStageFlags src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dst = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(
		cmd,
		srcStageFlags,
		destStageFlags,
		VK_FLAGS_NONE,
		0, nullptr,
		0, nullptr,
		1, &b);
}

namespace Textures {

void create (
	VkDevice device,
	VkDeviceSize size,
	VkSurfaceFormat format,
	uint32_t mipLevels,
	uint32_t width,
	uint32_t height,
	VkBuffer *stagingBuffer,
	VkDeviceMemory *stagingMem,
	VkImage *image,
	VkDeviceMemory *imageMemory
) {
	VkMemoryAllocateInfo memAllocInfo = {};
	VkMemoryRequirements memReqs = {};

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ASSERT_VULKAN(vkCreateBuffer(device, &bufferCreateInfo, nullptr, stagingBuffer));

	vkGetBufferMemoryRequirements(device, *stagingBuffer, &memReqs);
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memReqs.size;
	// TODO
	// memAllocInfo.memoryTypeIndex
	ASSERT_VULKAN(vkAllocateMemory(device, &memAllocInfo, nullptr, stagingMem));
	ASSERT_VULKAN(vkBindBufferMemory(device, *stagingBuffer, *stagingMem, 0));

	VkImageCreateInfo imageInfo;
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.mipLevels = mipLevels;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.extent = { texture.width, texture.height, 1 };
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ASSERT_VULKAN(vkCreateImage(device, &imageInfo, nullptr, image));

	vkGetImageMemoryRequirements(device, *image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	// TODO
	// memAllocInfo.memoryTypeIndex
	ASSERT_VULKAN(vkAllocateMemory(device, &memAllocInfo, nullptr, &imageMemory));
	ASSERT_VULKAN(vkBindImageMemory(device, *image, *imageMemory, 0));
}

void stage (
    VkDevice device,
    VkDeviceMemory stagingMem,
    const uint8_t *data,
	VkDeviceSize dataSize
) {
	uint8_t *data;
	ASSERT_VULKAN(vkMapMemory(device, stagingMem, 0, imageDataSize, 0, (void **)&data));
	std::copy(imageData, imageData + imageDataSize, data);
	vkUnmapMemory(device, stagingMem);
}

void transfer (
	VkCommandBuffer cmd,
	VkBuffer staging,
	VkImage image,
	const VkBufferImageCopy *copy,
	uint32_t copySize,
	uint32_t mipLevels
) {
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 1;

	setImageLayout(
		cmd,
		image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		subresourceRange);

	vkCommandBufferToImage(
		cmd,
		staging,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		copySize,
		copy);

	setImageLayout(
		cmd,
		image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		subresourceRange);
}

VkSampler sampler (
	VkDevice device,
	uint32_t mipLevels;
) {
	VkSampler sampler;
	VkSamplerCreateInfo s;
	s.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	/*
		TODO:
			* Texture filtering
			* Mipmaps
			* Anisotropic filtering
	*/
	s.magFilter = VK_FILTER_LINEAR;
	s.minFilter = VK_FILTER_LINEAR;
	s.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	s.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	s.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	s.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	s.mipLodBias = 0.0f;
	s.compareOp = VK_COMPARE_OP_NEVER;
	s.minLod = 0.0f;
	s.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	ASSERT_VULKAN(vkCreateSampler(device, &s, nullptr, &sampler));

	return sampler;
}

VkImageView view (
	VkDevice device,
	VkImage image,
	uint32_t mipLevels
) {
	VkImageView view;
	VkImageViewCreateInfo v = {};
	v.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	v.image = image;
	v.viewType = VK_IMAGE_VIEW_TYPE_2D;
	v.format = format;
	v.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	v.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	v.subresourceRange.baseMipLevel = 0;
	v.subresourceRange.baseArrayLayer = 0;
	v.subresourceRange.layerCount = 1;
	v.subresourceRange.levelCount = mipLevels;
	ASSERT_VULKAN(vkCreateImageView)
	return view;
}

}