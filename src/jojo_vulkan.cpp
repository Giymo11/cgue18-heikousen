//
// Created by benja on 4/28/2018.
//

#include "jojo_vulkan.hpp"

#include <array>

#include "debug_trap.h"

#include "jojo_utils.hpp"
#include "jojo_vulkan_data.hpp"
#include "jojo_vulkan_debug.hpp"

#define BREAK_BREAK_BREAK\
psnip_trap();\


VkResult createShaderModule(const VkDevice device, const std::vector<char> &code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = code.size();
    // guaranteed to be sound by spir-v
    shaderModuleCreateInfo.pCode = (uint32_t *) code.data();

    return vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, shaderModule);
}

VkResult createInstance(VkInstance *instance, std::vector<const char *> &usedExtensions) {
    VkApplicationInfo applicationInfo;
    // describes what kind of struct this is, because otherwise the type info is lost when passed to the driver
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // used for extensions
    applicationInfo.pNext = nullptr;
    applicationInfo.pApplicationName = "heikousen";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "JoJo Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    const std::vector<const char *> usedLayers = {
#ifdef ENABLE_VALIDATION
            "VK_LAYER_LUNARG_standard_validation"
#endif
    };

#ifdef ENABLE_VALIDATION
    usedExtensions.push_back ("VK_EXT_debug_report");
#endif

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    // Flags are reserved for future use
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    // Layers are used for debugging, profiling, error handling,
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(usedLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = usedLayers.data();
    // Extensions are used to extend vulkan functionality
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(usedExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = usedExtensions.data();


    // vulkan instance is similar to OpenGL context
    // by telling vulkan which extensions we plan on using, it can disregard all others
    // -> performance gain in comparison to openGL
    return vkCreateInstance(&instanceCreateInfo, nullptr, instance);
}

VkResult createLogicalDevice(const VkPhysicalDevice chosenDevice,
                             VkDevice *device,
                             const uint32_t chosenQueueFamilyIndex) {

    float queuePriorities[]{1.0f};

    VkDeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = chosenQueueFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1; // TODO: check how many families are supported, 4 would be better
    deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

    VkPhysicalDeviceFeatures usedFeatures = {};

    const std::vector<const char *> usedDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(usedDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = usedDeviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &usedFeatures;

    return vkCreateDevice(chosenDevice, &deviceCreateInfo, nullptr, device);
}

VkResult checkSurfaceSupport(const VkPhysicalDevice chosenDevice, const VkSurfaceKHR surface,
                             const uint32_t chosenQueueFamilyIndex) {
    VkBool32 surfaceSupport = VK_FALSE;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(chosenDevice, chosenQueueFamilyIndex, surface,
                                                           &surfaceSupport);
    if (!surfaceSupport) {
        std::cerr << "Surface not supported!" << std::endl;
        return result; // TODO: debug trap
    }
    return result;
}


VkResult createSwapchain(const VkDevice device,
                         const VkSurfaceKHR surface,
                         const VkSwapchainKHR oldSwapchain,
                         VkSwapchainKHR *swapchain,
                         const VkFormat chosenImageFormat,
                         uint32_t width,
                         uint32_t height,
                         bool vsync) {
    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = 3; // TODO: check if valid via surfaceCapabilitiesKHR.maxImageCount
    swapchainCreateInfo.imageFormat = chosenImageFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;    // TODO: check if valid via surfaceFormats
    swapchainCreateInfo.imageExtent = VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if(vsync) {
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO: maybe mailbox?
    } else {
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // TODO: maybe mailbox?
    }

    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

    return vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, swapchain);
}


VkResult createImageView(const VkDevice device, const VkImage swapchainImage,
                         VkImageView *imageView,
                         const VkFormat chosenImageFormat, const VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = chosenImageFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;

    imageViewCreateInfo.image = swapchainImage;
    return vkCreateImageView(device, &imageViewCreateInfo, nullptr, imageView);
}

VkResult
createShaderStageCreateInfo(const VkDevice device, const std::string &filename,
                            const VkShaderStageFlagBits &shaderStage,
                            VkPipelineShaderStageCreateInfo *shaderStageCreateInfo, VkShaderModule *shaderModule) {
    auto shaderCode = readFile(filename);

    VkResult result = createShaderModule(device, shaderCode, shaderModule);

    shaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo->pNext = nullptr;
    shaderStageCreateInfo->flags = 0;
    shaderStageCreateInfo->stage = shaderStage;
    shaderStageCreateInfo->module = *shaderModule;
    shaderStageCreateInfo->pName = "main";
    shaderStageCreateInfo->pSpecializationInfo = nullptr;

    return result;
}


VkResult createRenderpass(const VkDevice device, VkRenderPass *renderPass, const VkFormat chosenImageFormat,
                          const VkFormat chosenDepthFormat) {

    VkAttachmentDescription colorAttachmentDescription;
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = chosenImageFormat;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference;
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    // because vulkan provides two additional subpasses to transform the image layouts (see attachment description)
    // one from initial to ours (see colorAttachmentReference), and then from ours to final
    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;


    std::array<VkAttachmentDescription, 1> attachments = {colorAttachmentDescription};

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    return vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, renderPass);
}


VkResult createDescriptorSetLayout(const VkDevice device, VkDescriptorSetLayout *descriptorSetLayout) {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

    return vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, descriptorSetLayout);
}

VkResult createPipelineLayout(const VkDevice device, const VkDescriptorSetLayout *descriptorSetLayout,
                              VkPipelineLayout *pipelineLayout) {

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    return vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, pipelineLayout);
}

VkResult createPipeline (
    const VkDevice                          device,
    const VkPipelineShaderStageCreateInfo  *shaderStages,
    const VkRenderPass                      renderPass,
    const VkPipelineLayout                  pipelineLayout,
    VkPipeline                             *pipeline,
    const int                               width,
    const int                               height,
    const std::vector<VkVertexInputBindingDescription>     &vertexBindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>   &vertexAttributeDescriptions,
    const std::vector<VkPipelineColorBlendAttachmentState> &blendAttachments, 
    const bool                              isWireframeEnabled
) {
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions.size();
    vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescriptions.size();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();


    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.pNext = nullptr;
    inputAssemblyStateCreateInfo.flags = 0;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;


    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;


    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};


    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = nullptr;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;


    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.pNext = nullptr;
    rasterizationStateCreateInfo.flags = 0;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = isWireframeEnabled ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;  // backface culling!
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStateCreateInfo.lineWidth = 1.0f;


    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.pNext = nullptr;
    multisampleStateCreateInfo.flags = 0;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.pNext = nullptr;
    colorBlendStateCreateInfo.flags = 0;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendStateCreateInfo.attachmentCount = (uint32_t)blendAttachments.size();
    colorBlendStateCreateInfo.pAttachments = blendAttachments.data();
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;


    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;


    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;


    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;   // used for pipeline derivation
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pTessellationState = nullptr;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, pipeline);
}


VkResult createFramebuffer(const VkDevice device, const VkRenderPass renderPass,
                           const VkImageView swapChainImageView, const VkImageView depthImageView,
                           VkFramebuffer *framebuffer,
                           uint32_t width, uint32_t height) {

    std::array<VkImageView, 1> attachments = {
            swapChainImageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = nullptr;
    framebufferCreateInfo.flags = 0;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    return vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, framebuffer);
}


VkResult createCommandPool(const VkDevice device, VkCommandPool *commandPool, const uint32_t chosenQueueFamilyIndex) {
    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = chosenQueueFamilyIndex;
    // the chosen queue has to be chosen to support Graphics Queue

    return vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, commandPool);
}


VkResult allocateCommandBuffers (
    const VkDevice                device,
    const VkCommandPool           commandPool,
    std::vector<VkCommandBuffer> &commandBuffers,
    const uint32_t                numberOfImagesInSwapchain
) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = numberOfImagesInSwapchain;


    commandBuffers.resize(numberOfImagesInSwapchain);
    return vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data());
}

VkResult beginCommandBuffer(const VkCommandBuffer commandBuffer) {
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    // to be able to submit the command buffer to a queue that is (still) holding it
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    return vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
}

VkResult createSemaphore(const VkDevice device, VkSemaphore *semaphore) {
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    return vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, semaphore);
}

VkResult createDescriptorPool(const VkDevice device,
                              VkDescriptorPool *descriptorPool,
                              uint32_t uniformCount,
                              uint32_t dynamicUniformCount,
                              uint32_t samplerCount) {

    VkDescriptorPoolSize uniformDescriptorPoolSize;
    uniformDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformDescriptorPoolSize.descriptorCount = uniformCount;

    VkDescriptorPoolSize dynamicUniformDescriptorPoolSize;
    dynamicUniformDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    dynamicUniformDescriptorPoolSize.descriptorCount = dynamicUniformCount;

    VkDescriptorPoolSize samplerDescriptorPoolSize;
    samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorPoolSize.descriptorCount = samplerCount;

    std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes = {
            uniformDescriptorPoolSize,
            dynamicUniformDescriptorPoolSize,
            samplerDescriptorPoolSize
    };


    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = uniformCount + dynamicUniformCount + samplerCount;
    descriptorPoolCreateInfo.poolSizeCount = (uint32_t)descriptorPoolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

    return vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, descriptorPool);
}

VkResult allocateDescriptorSet(const VkDevice device, const VkDescriptorPool descriptorPool,
                               const VkDescriptorSetLayout descriptorSetLayout,
                               VkDescriptorSet *descriptorSet) {
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    return vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSet);
}


