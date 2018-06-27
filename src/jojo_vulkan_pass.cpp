#include <array>
#include "jojo_vulkan_pass.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_data.hpp"
#include "jojo_vulkan.hpp"
#include "Rendering/DescriptorSets.h"
#include <glm/gtx/fast_trigonometry.hpp>

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
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

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

static void createMrtPass (
    const VkDevice      device,
    const VmaAllocator  allocator,
    const VkFormat      depthFormat,
    const uint32_t      width,
    const uint32_t      height,
    RenderPass         *pass
) {
    const uint32_t numAtt = 5;
    const uint32_t numCol = 4;
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
    pass->attachments.resize (numAtt);

    /* POSITIONS */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[0]
    );

    /* NORMALS & DOF INFO */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[1]
    );

    /* ALBEDO */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R8G8B8A8_UNORM,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[2]
    );

    /* MATERIAL PARAMETERS */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R8G8B8A8_UNORM,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[3]
    );

    /* DEPTH */
    allocAttachment (
        device, allocator,
        depthFormat, width, height,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        &attachments[4]
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
    color[3].attachment = 3;
    color[3].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    depth.attachment    = 4;
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

static void createDeferredPass (
    const VkDevice      device,
    const VmaAllocator  allocator,
    const uint32_t      width,
    const uint32_t      height,
    RenderPass         *mrtPass,
    RenderPass         *pass
) {
    const uint32_t numAtt = 2;
    const uint32_t numCol = 1;

    std::array<VkAttachmentDescription, numAtt> att = {};
    std::array<VkAttachmentReference, numCol>   color = {};
    VkAttachmentReference                       depth = {};

    VkSubpassDescription               subpass = {};
    std::array<VkSubpassDependency, 2> dependencies = {};
    VkRenderPassCreateInfo             renderPassInfo = {};
    std::array<VkImageView, numAtt>    attachViews = {};
    VkFramebufferCreateInfo            fbCreateInfo = {};
    VkSamplerCreateInfo                samplerInfo = {};

    auto &attachments = pass->attachments;
    pass->attachments.resize (numAtt);

    /* ALBEDO */
    allocAttachment (
        device, allocator,
        VK_FORMAT_R8G8B8A8_UNORM,
        width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &attachments[0]
    );

    /* DEPTH */
    attachments[1] = mrtPass->attachments.back ();

    for (uint32_t i = 0; i < numAtt; i++) {
        att[i].format = attachments[i].format;
        attachViews[i] = attachments[i].imageView;
    }

    for (uint32_t i = 0; i < numAtt; ++i) {
        att[i].samples = VK_SAMPLE_COUNT_1_BIT;
        att[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        att[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    att.back ().loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    att.back ().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.back ().initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    att.back ().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    color[0].attachment = 0;
    color[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    depth.attachment = 1;
    depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = color.data ();
    subpass.colorAttachmentCount = (uint32_t)color.size ();
    subpass.pDepthStencilAttachment = &depth;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = att.data ();
    renderPassInfo.attachmentCount = numAtt;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pDependencies = dependencies.data ();
    renderPassInfo.dependencyCount = (uint32_t)dependencies.size ();

    ASSERT_VULKAN (vkCreateRenderPass (
        device, &renderPassInfo,
        nullptr, &pass->pass
    ));

    fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.width = width;
    fbCreateInfo.height = height;
    fbCreateInfo.layers = 1;
    fbCreateInfo.renderPass = pass->pass;
    fbCreateInfo.pAttachments = attachViews.data ();
    fbCreateInfo.attachmentCount = numAtt;

    ASSERT_VULKAN (vkCreateFramebuffer (
        device, &fbCreateInfo,
        nullptr, &pass->fb
    ));

    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

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

void allocPassStorage (
    const VkDevice       device,
    const VkCommandPool  commandPool,
    const uint32_t       numCmdBuffers,
    PassStorage         *passes
) {
    ASSERT_VULKAN (allocateCommandBuffers (
        device, commandPool,
        passes->mrtCmd,
        numCmdBuffers
    ));

    ASSERT_VULKAN (allocateCommandBuffers (
        device, commandPool,
        passes->deferredCmd,
        numCmdBuffers
    ));

    ASSERT_VULKAN (createSemaphore (
        device, &passes->transferSema
    ));

    ASSERT_VULKAN (createSemaphore (
        device, &passes->mrtSema
    ));

    ASSERT_VULKAN (createSemaphore (
        device, &passes->deferredSema
    ));
}

void freePassStorage (
    const VkDevice       device,
    const VkCommandPool  commandPool,
    PassStorage         *passes
) {
    vkFreeCommandBuffers (
        device, commandPool,
        (uint32_t)passes->deferredCmd.size (),
        passes->deferredCmd.data ()
    );

    vkFreeCommandBuffers (
        device, commandPool,
        (uint32_t)passes->mrtCmd.size (),
        passes->mrtCmd.data ()
    );

    vkDestroySemaphore (
        device, passes->deferredSema,
        nullptr
    );

    vkDestroySemaphore (
        device, passes->mrtSema,
        nullptr
    );

    vkDestroySemaphore (
        device, passes->transferSema,
        nullptr
    );
}

void allocPasses (
    const VkDevice                   device,
    const VmaAllocator               allocator,
    const Rendering::DescriptorSets *descriptors,
    const uint32_t                   width,
    const uint32_t                   height,
    PassStorage                     *passes
) {
    {
        const auto set            = Rendering::Set::Deferred;
        auto       pass           = &passes->mrtPass;

        createMrtPass (
            device, allocator,
            passes->depthFormat,
            width, height,
            pass
        );

        VkDescriptorImageInfo info;
        info.sampler = pass->sampler;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        const auto numAtt = (uint32_t)pass->attachments.size () - 1;
        for (uint32_t i = 0; i < numAtt; i++) {
            info.imageView = pass->attachments[i].imageView;
            descriptors->update (set, i + 1, info);
        }

        info.imageView = pass->attachments[1].imageView;
        descriptors->update (Rendering::Set::Dof, 2, info);
    }

    {
        const auto set = Rendering::Set::Dof;
        auto       pass = &passes->deferredPass;

        createDeferredPass (
            device, allocator,
            width, height,
            &passes->mrtPass, pass
        );

        VkDescriptorImageInfo info;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        info.sampler   = pass->sampler;
        info.imageView = pass->attachments[0].imageView;
        descriptors->update (set, 1, info);
    }
}

void freePasses (
    const VkDevice      device,
    const VmaAllocator  allocator,
    PassStorage        *passes
) {
    passes->deferredPass.attachments.resize (1);
    freePass (device, allocator, &passes->deferredPass);
    freePass (device, allocator, &passes->mrtPass);
}

void calcDoFTaps (
    const Config       *config,
    Data::DepthOfField *depth
) {
    const float RAD_SCALE = 0.5f;
    const int   TAP_COUNT = config->dofTaps;
    float       angle     = 0.0f;
    float       radius    = RAD_SCALE;
    float       width     = 1.f / (float)config->width;
    float       height    = 1.f / (float)config->height;

    for (int i = 0; i < TAP_COUNT; i++) {
        glm::vec4 &t = depth->taps[i];
        t.x = glm::fastCos(angle) * width * radius;
        t.y = glm::fastSin(angle) * height * radius;
        t.z = radius - 0.5f;
        t.w = (float)(i + 1);

        radius += RAD_SCALE/radius;
        angle  += 2.39996323;
    }

    depth->tapCount = TAP_COUNT;
}

}