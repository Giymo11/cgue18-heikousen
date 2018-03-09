#include <iostream>
#include <fstream>
#include <vector>

// #include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "debug_trap.h"


#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS)\
    psnip_trap();\


VkInstance instance;
VkSurfaceKHR surface;
VkDevice device;
VkSwapchainKHR swapchain;
VkQueue queue;

std::vector<VkImageView> imageViews;
std::vector<VkFramebuffer> framebuffers;
std::vector<VkCommandBuffer> commandBuffers;

VkShaderModule shaderModuleVert;
VkShaderModule shaderModuleFrag;

VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline pipeline;
VkCommandPool commandPool;

VkSemaphore semaphoreImageAvailable;
VkSemaphore semaphoreRenderingDone;


GLFWwindow *window;

// TODO: read from resource
const uint32_t width = 1200;
const uint32_t height = 720;


void printStats(VkPhysicalDevice &device);

VkResult createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = code.size();
    // guaranteed to be sound by spir-v
    shaderModuleCreateInfo.pCode = (uint32_t *) code.data();

    return vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, shaderModule);
}


std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file) {
        size_t fileSize = file.tellg();
        std::vector<char> fileBuffer(fileSize);
        file.seekg(0);
        file.read(fileBuffer.data(), fileSize);
        file.close();
        return fileBuffer;
    } else {
        throw std::runtime_error("Failed to open file");
    }
}


void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "heikousen", nullptr, nullptr);
}

VkResult createInstance() {
    VkResult result;


    VkApplicationInfo applicationInfo = {};
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
            "VK_LAYER_LUNARG_standard_validation"
    };


    uint32_t indexOfNumberOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&indexOfNumberOfGlfwExtensions);

    // vector constructor parameters take begin and end pointer
    std::vector<const char *> usedExtensions(glfwExtensions, glfwExtensions + indexOfNumberOfGlfwExtensions);
    // TODO: add validation layer callback extension -
    // TODO: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers#page_Message_callback


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
    instanceCreateInfo.enabledExtensionCount = usedExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = usedExtensions.data();


    // vulkan instance is similar to OpenGL context
    // by telling vulkan which extensions we plan on using, it can disregard all others
    // -> performance gain in comparison to openGL
    return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
}

void printInstanceLayers() {
    uint32_t numberOfLayers = 0;
    vkEnumerateInstanceLayerProperties(&numberOfLayers, nullptr);

    std::vector<VkLayerProperties> layers;
    layers.resize(numberOfLayers);
    vkEnumerateInstanceLayerProperties(&numberOfLayers, layers.data());

    std::cout << std::endl << "Amount of instance layers: " << numberOfLayers << std::endl << std::endl;
    for (size_t i = 0; i < numberOfLayers; ++i) {
        std::cout << "Name:            " << layers[i].layerName << std::endl;
        std::cout << "Spec Version:    " << layers[i].specVersion << std::endl;
        std::cout << "Description:     " << layers[i].description << std::endl << std::endl;
    }
}

void printInstanceExtensions() {
    uint32_t numberOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, nullptr);

    std::vector<VkExtensionProperties> extensions;
    extensions.resize(numberOfExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, extensions.data());

    std::cout << std::endl << "Amount of instance extensions: " << numberOfExtensions << std::endl << std::endl;
    for (size_t i = 0; i < numberOfExtensions; ++i) {
        std::cout << "Name:            " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version:    " << extensions[i].specVersion << std::endl << std::endl;
    }
}

VkResult createLogicalDevice(VkPhysicalDevice chosenDevice, uint32_t chosenQueueFamilyIndex) {

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
    deviceCreateInfo.enabledExtensionCount = usedDeviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = usedDeviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &usedFeatures;

    return vkCreateDevice(chosenDevice, &deviceCreateInfo, nullptr, &device);
}

VkResult checkSurfaceSupport(VkPhysicalDevice chosenDevice, uint32_t chosenQueueFamilyIndex) {
    VkBool32 surfaceSupport = VK_FALSE;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(chosenDevice, chosenQueueFamilyIndex, surface,
                                                           &surfaceSupport);
    if (!surfaceSupport) {
        std::cerr << "Surface not supported!" << std::endl;
        psnip_trap();
    }
    return result;
}

VkResult createSwapchain(VkFormat chosenImageFormat) {
    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = 3; // TODO: check if valid via surfaceCapabilitiesKHR.maxImageCount
    swapchainCreateInfo.imageFormat = chosenImageFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;    // TODO: check if valid via surfaceFormats
    swapchainCreateInfo.imageExtent = VkExtent2D{width, height};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO: maybe mailbox?
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // TODO: for resizing

    return vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
}

VkResult createImageView(VkFormat chosenImageFormat, VkImage swapchainImage, VkImageView *imageView) {
    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    // imageViewCreateInfo.image = swapchainImages[0];
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = chosenImageFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;

    imageViewCreateInfo.image = swapchainImage;
    return vkCreateImageView(device, &imageViewCreateInfo, nullptr, imageView);
}

VkResult
createShaderStageCreateInfo(const std::string &filename, VkPipelineShaderStageCreateInfo *shaderStageCreateInfo,
                            VkShaderModule *shaderModule, const VkShaderStageFlagBits &shaderStage) {
    auto shaderCode = readFile(filename);

    VkResult result = createShaderModule(shaderCode, shaderModule);

    shaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo->pNext = nullptr;
    shaderStageCreateInfo->flags = 0;
    shaderStageCreateInfo->stage = shaderStage;
    shaderStageCreateInfo->module = *shaderModule;
    shaderStageCreateInfo->pName = "main";
    shaderStageCreateInfo->pSpecializationInfo = nullptr;

    return result;
}

VkResult createRenderpass(VkFormat chosenImageFormat) {

    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = chosenImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    // because vulkan provides two additional subpasses to transform the image layouts (see attachment description)
    // one from initial to ours (see attachmentReference), and then from ours to final
    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;


    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    return vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
}

VkResult createPipeline(const VkPipelineShaderStageCreateInfo *shaderStages) {

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;


    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.pNext = nullptr;
    inputAssemblyStateCreateInfo.flags = 0;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;


    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;


    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {width, height};


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
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;  // backface culling!
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // TODO: check if this is correct
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


    VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
    colorBlendAttachmentState.blendEnable = VK_TRUE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;


    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.pNext = nullptr;
    colorBlendStateCreateInfo.flags = 0;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;


    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    ASSERT_VULKAN(result)


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
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    ASSERT_VULKAN(result)
}

VkResult createFramebuffer(VkImageView *attachments, VkFramebuffer *framebuffer) {
    VkFramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = nullptr;
    framebufferCreateInfo.flags = 0;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    return vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, framebuffer);
}

VkResult createCommandPool(uint32_t chosenQueueFamilyIndex) {
    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = chosenQueueFamilyIndex;
    // the chosen queue has to be chosen to support Graphics Queue

    return vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
}

VkResult allocateCommandBuffers(uint32_t numberOfImagesInSwapchain) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = numberOfImagesInSwapchain;


    commandBuffers.resize(numberOfImagesInSwapchain);
    return vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data());
}

VkResult recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer) {
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    // to be able to submit the command buffer to a queue that is (still) holding it
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    ASSERT_VULKAN(result)

    VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {width, height};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    // _INLINE means to only use primary command buffers
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    return vkEndCommandBuffer(commandBuffer);
}

VkResult createSemaphore(VkSemaphore *semaphore) {
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    return vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, semaphore);
}

void startVulkan() {
    VkResult result;

    result = createInstance();
    ASSERT_VULKAN(result)

    printInstanceLayers();
    printInstanceExtensions();

    result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    ASSERT_VULKAN(result)

    uint32_t numberOfPhysicalDevices = 0;
    // if passed nullptr as third parameter, outputs the number of GPUs to the second parameter
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, nullptr);
    ASSERT_VULKAN(result)

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(numberOfPhysicalDevices);
    // actually enumerates the GPUs for use
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, physicalDevices.data());
    ASSERT_VULKAN(result)

    std::cout << std::endl << "GPUs Found: " << numberOfPhysicalDevices << std::endl << std::endl;
    for (size_t i = 0; i < numberOfPhysicalDevices; ++i)
        printStats(physicalDevices[i]);

    auto chosenDevice = physicalDevices[0];     // TODO: choose right physical device
    uint32_t chosenQueueFamilyIndex = 0;        // TODO: choose the best queue family

    result = createLogicalDevice(chosenDevice, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    vkGetDeviceQueue(device, chosenQueueFamilyIndex, 0, &queue);

    result = checkSurfaceSupport(chosenDevice, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    auto chosenImageFormat = VK_FORMAT_B8G8R8A8_UNORM;   // TODO: check if valid via surfaceFormats[i].format

    result = createSwapchain(chosenImageFormat);
    ASSERT_VULKAN(result)

    uint32_t numberOfImagesInSwapchain = 0;
    result = vkGetSwapchainImagesKHR(device, swapchain, &numberOfImagesInSwapchain, nullptr);
    ASSERT_VULKAN(result)

    std::vector<VkImage> swapchainImages;
    swapchainImages.resize(numberOfImagesInSwapchain);
    result = vkGetSwapchainImagesKHR(device, swapchain, &numberOfImagesInSwapchain, swapchainImages.data());
    ASSERT_VULKAN(result)

    imageViews.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createImageView(chosenImageFormat, swapchainImages[i], &(imageViews[i]));
        ASSERT_VULKAN(result)
    }

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    result = createShaderStageCreateInfo("../shader/shader.vert.spv", &shaderStageCreateInfoVert, &shaderModuleVert,
                                         VK_SHADER_STAGE_VERTEX_BIT);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    result = createShaderStageCreateInfo("../shader/shader.frag.spv", &shaderStageCreateInfoFrag, &shaderModuleFrag,
                                         VK_SHADER_STAGE_FRAGMENT_BIT);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStages[] = {shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

    result = createRenderpass(chosenImageFormat);
    ASSERT_VULKAN(result)

    result = createPipeline(shaderStages);
    ASSERT_VULKAN(result)

    framebuffers.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createFramebuffer(&(imageViews[i]), &(framebuffers[i]));
        ASSERT_VULKAN(result)
    }

    result = createCommandPool(chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    result = allocateCommandBuffers(numberOfImagesInSwapchain);
    ASSERT_VULKAN(result)

    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = recordCommandBuffer(commandBuffers[i], framebuffers[i]);
        ASSERT_VULKAN(result)
    }

    result = createSemaphore(&semaphoreImageAvailable);
    ASSERT_VULKAN(result)
    result = createSemaphore(&semaphoreRenderingDone);
    ASSERT_VULKAN(result)
}

void drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(),
                          semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);

    VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphoreImageAvailable;
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphoreRenderingDone;

    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result)

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(queue, &presentInfo);
    ASSERT_VULKAN(result)
}

void gameloop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

void shutdownVulkan() {
    // block until vulkan has finished
    vkDeviceWaitIdle(device);

    vkDestroySemaphore(device, semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto &framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyShaderModule(device, shaderModuleVert, nullptr);
    vkDestroyShaderModule(device, shaderModuleFrag, nullptr);
    for (auto &imageView : imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void shutdownGlfw() {
    glfwDestroyWindow(window);
}

int main() {
    std::cout << "Hello World!" << std::endl << std::endl;

    startGlfw();
    startVulkan();

    gameloop();

    shutdownVulkan();
    shutdownGlfw();

    return 0;
}

void printStats(VkPhysicalDevice &device) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    std::cout << "GPU Name: " << properties.deviceName << std::endl;
    uint32_t version = properties.apiVersion;
    std::cout << "Max API Version:           " <<
              VK_VERSION_MAJOR(version) << "." <<
              VK_VERSION_MINOR(version) << "." <<
              VK_VERSION_PATCH(version) << std::endl;

    version = properties.driverVersion;
    std::cout << "Driver Version:            " <<
              VK_VERSION_MAJOR(version) << "." <<
              VK_VERSION_MINOR(version) << "." <<
              VK_VERSION_PATCH(version) << std::endl;

    std::cout << "Vendor ID:                 " << properties.vendorID << std::endl;
    std::cout << "Device ID:                 " << properties.deviceID << std::endl;

    auto deviceType = properties.deviceType;
    auto deviceTypeDescription =
            (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ?
             "discrete" :
             (deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ?
              "integrated" :
              "other"
             )
            );
    std::cout << "Device Type:               " << deviceTypeDescription << " (type " << deviceType << ")" << std::endl;
    std::cout << "discreteQueuePrioritis:    " << properties.limits.discreteQueuePriorities << std::endl;


    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    // ...
    std::cout << "can it do multi-viewport:  " << features.multiViewport << std::endl;


    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
    // ...


    uint32_t numberOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    queueFamilyProperties.resize(numberOfQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, queueFamilyProperties.data());

    std::cout << "Amount of Queue Families:  " << numberOfQueueFamilies << std::endl << std::endl;

    for (size_t i = 0; i < numberOfQueueFamilies; ++i) {
        std::cout << "Queue Family #" << i << std::endl;
        auto flags = queueFamilyProperties[i].queueFlags;
        std::cout << "VK_QUEUE_GRAPHICS_BIT " << ((flags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT  " << ((flags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT " << ((flags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "Queue Count:          " << queueFamilyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << queueFamilyProperties[i].timestampValidBits << std::endl;
    }


    VkSurfaceCapabilitiesKHR surfaceCapabilitiesKHR;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilitiesKHR);
    std::cout << std::endl << std::endl << "Surface Capabilities" << std::endl << std::endl;
    std::cout << "minImageCount: " << surfaceCapabilitiesKHR.minImageCount << std::endl;
    std::cout << "maxImageCount: " << surfaceCapabilitiesKHR.maxImageCount << std::endl; // a 0 means no limit
    std::cout << "maxImageArrayLayers: " << surfaceCapabilitiesKHR.maxImageArrayLayers << std::endl;
    // ...


    uint32_t numberOfSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numberOfSurfaceFormats, nullptr);

    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    surfaceFormats.resize(numberOfSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numberOfSurfaceFormats, surfaceFormats.data());

    std::cout << std::endl << "Amount of Surface Formats: " << numberOfSurfaceFormats << std::endl;
    for (size_t i = 0; i < numberOfSurfaceFormats; ++i) {
        std::cout << "Format: " << surfaceFormats[i].format << std::endl;
    }


    uint32_t numberOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numberOfPresentationModes, nullptr);

    std::vector<VkPresentModeKHR> presentationModes;
    presentationModes.resize(numberOfPresentationModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numberOfPresentationModes, presentationModes.data());

    std::cout << std::endl << "Amount of Presentation Modes: " << numberOfPresentationModes << std::endl;
    for (size_t i = 0; i < numberOfPresentationModes; ++i) {
        std::cout << "Mode " << presentationModes[i] << std::endl;
    }


    std::cout << std::endl;
}
