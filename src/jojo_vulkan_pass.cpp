#include <array>
#include "jojo_vulkan_pass.hpp"
#include "jojo_vulkan_utils.hpp"

namespace Pass {

static void allocAttachment (
    const VkDevice              device,
    const VmaAllocator          allocator,
    const VkFormat              format,
    const uint32_t              width,
    const uint32_t              height,
    const VkImageUsageFlagBits  usage,
    Attachment                  *att
) {
    VkImageAspectFlags aspectMask = 0;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    VkImageCreateInfo iinfo = {};
    iinfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    iinfo.imageType     = VK_IMAGE_TYPE_2D;
    iinfo.format        = format;
    iinfo.extent.width  = width;
    iinfo.extent.height = height;
    iinfo.extent.depth  = 1;
    iinfo.mipLevels     = 1;
    iinfo.arrayLayers   = 1;
    iinfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    iinfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    iinfo.usage         = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ASSERT_VULKAN (vmaCreateImage (
        allocator, &iinfo,
        &allocInfo, &att->image,
        &att->memory, nullptr
    ));

    VkImageViewCreateInfo ivinfo = {};
    ivinfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivinfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    ivinfo.format                = format;
    ivinfo.image                 = att->image;
    ivinfo.subresourceRange                = {};
    ivinfo.subresourceRange.aspectMask     = aspectMask;
    ivinfo.subresourceRange.baseMipLevel   = 0;
    ivinfo.subresourceRange.levelCount     = 1;
    ivinfo.subresourceRange.baseArrayLayer = 0;
    ivinfo.subresourceRange.layerCount = 1;

    ASSERT_VULKAN (vkCreateImageView (
        device, &ivinfo,
        nullptr, &att->imageView
    ));

    att->format = format;
}

static void freeAttachment (
    const VkDevice      device,
    const VmaAllocator  allocator,
    Attachment         *att
) {
    vkDestroyImageView (device, att->imageView, nullptr);
    vmaDestroyImage (allocator, att->image, att->memory);
}

static void createDeferredPass (
    const VkDevice      device,
    const VmaAllocator  allocator,
    const VkFormat      depthFormat,
    const uint32_t      width,
    const uint32_t      height,
    RenderPass         *pass
) {
    const uint32_t numAtt = 4;
    const uint32_t numCol = 3;
    const uint32_t numDep = 1;

    std::array<VkAttachmentDescription, numAtt> att   = {};
    std::array<VkAttachmentReference, numCol>   color = {};
    VkAttachmentReference                       depth = {};

    VkSubpassDescription               subpass        = {};
    std::array<VkSubpassDependency, 2> dependencies   = {};
    VkRenderPassCreateInfo             renderPassInfo = {};
    std::array<VkImageView, numAtt>    attachViews    = {};
    VkFramebufferCreateInfo            fbCreateInfo   = {};
    VkSamplerCreateInfo                samplerInfo    = {};

    auto &attachments = pass->attachments;
    pass->attachments.resize (4);

    /* POSITIONS */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[0]
    );

    /* NORMALS */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[1]
    );

    /* ALBEDO */
    /* TODO: HDR */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R8G8B8A8_UNORM,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[2]
    );

    /* DEPTH */
    allocAttachment (
        device, allocator,
        depthFormat, width, height,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        &attachments[3]
    );

    for (uint32_t i = 0; i < numAtt; i++) {
        att[i].format  = attachments[i].format;
        attachViews[i] = attachments[i].imageView;
    }

    for (uint32_t i = 0; i < numAtt; ++i) {
        att[i].samples        = VK_SAMPLE_COUNT_1_BIT;
        att[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        att[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        att[i].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    att.back ().finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    color[0].attachment = 0;
    color[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color[1].attachment = 1;
    color[1].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color[2].attachment = 2;
    color[2].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    depth.attachment    = 3;
    depth.layout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments       = color.data ();
    subpass.colorAttachmentCount    = (uint32_t)color.size ();
    subpass.pDepthStencilAttachment = &depth;
    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments     = att.data ();
    renderPassInfo.attachmentCount  = numAtt;
    renderPassInfo.pSubpasses       = &subpass;
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pDependencies    = dependencies.data ();
    renderPassInfo.dependencyCount  = (uint32_t)dependencies.size ();

    ASSERT_VULKAN (vkCreateRenderPass (
        device, &renderPassInfo,
        nullptr, &pass->pass
    ));

    fbCreateInfo.sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.width              = width;
    fbCreateInfo.height             = height;
    fbCreateInfo.layers             = 1;
    fbCreateInfo.renderPass         = pass->pass;
    fbCreateInfo.pAttachments       = attachViews.data ();
    fbCreateInfo.attachmentCount    = numAtt;

    ASSERT_VULKAN (vkCreateFramebuffer (
        device, &fbCreateInfo,
        nullptr, &pass->fb
    ));

    samplerInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter     = VK_FILTER_NEAREST;
    samplerInfo.minFilter     = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV  = samplerInfo.addressModeU;
    samplerInfo.addressModeW  = samplerInfo.addressModeU;
    samplerInfo.mipLodBias    = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod        = 0.0f;
    samplerInfo.maxLod        = 1.0f;
    samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    ASSERT_VULKAN (vkCreateSampler (
        device, &samplerInfo,
        nullptr, &pass->sampler
    ));
}

static void freePass (
    const VkDevice      device,
    const VmaAllocator  allocator,
    RenderPass         *pass
) {
    vkDestroySampler (device, pass->sampler, nullptr);
    vkDestroyFramebuffer (device, pass->fb, nullptr);
    vkDestroyRenderPass (device, pass->pass, nullptr);

    for (auto &att : pass->attachments)
        freeAttachment (device, allocator, &att);
}

void allocPasses (
    const VkDevice      device,
    const VmaAllocator  allocator,
    const uint32_t      width,
    const uint32_t      height,
    PassStorage        *passes
) {
    createDeferredPass (
        device, allocator,
        passes->depthFormat,
        width, height,
        &passes->deferredPass
    );
}

void freePasses (
    const VkDevice      device,
    const VmaAllocator  allocator,
    PassStorage        *passes
) {
    freePass (device, allocator, &passes->deferredPass);
}

}