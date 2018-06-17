#include "Common/ErrorHandling.h"
#include "Rendering/DescriptorSets.h"

namespace Rendering {

DescriptorSets::DescriptorSets (
    VkDevice device
)
    : device (device)
{
    createLayouts ();
}

DescriptorSets::~DescriptorSets ()
{
    for (const auto &layout : layouts)
        vkDestroyDescriptorSetLayout (device, layout, nullptr);
}

void DescriptorSets::allocate (
    VkDescriptorPool pool
) {
    std::vector<VkDescriptorSetLayout> vulkanLayouts (layouts.size ());
    std::copy (layouts.begin (), layouts.end (), vulkanLayouts.begin ());

    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pool;
    info.descriptorSetCount = static_cast<uint32_t>(layouts.size ());
    info.pSetLayouts = vulkanLayouts.data ();

    descriptorSets.resize (layouts.size ());
    auto result = vkAllocateDescriptorSets (device, &info, descriptorSets.data ());
    CHECK_VK (result, "Could not allocate descriptor sets");
    req.clear ();
}

void DescriptorSets::update (
    Set set,
    uint32_t binding,
    VkDescriptorType type,
    VkDescriptorBufferInfo info
) const {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[static_cast<size_t>(set)];
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;
    vkUpdateDescriptorSets (device, 1, &write, 0, nullptr);
}

void DescriptorSets::update (
    Set set,
    uint32_t binding,
    VkDescriptorImageInfo info
) const {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[static_cast<size_t>(set)];
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &info;
    vkUpdateDescriptorSets (device, 1, &write, 0, nullptr);
}

VkDescriptorSet DescriptorSets::set (
    Set s
) const {
    return descriptorSets[static_cast<size_t>(s)];
}

VkDescriptorSetLayout DescriptorSets::layout (
    Set s
) const {
    return layouts[static_cast<size_t>(s)];
}

const std::unordered_map<VkDescriptorType, uint32_t> &
DescriptorSets::requirements () const
{
    return req;
};

void DescriptorSets::addLayout (
    std::vector<VkDescriptorSetLayoutBinding> &layout,
    uint32_t binding,
    VkDescriptorType type,
    VkShaderStageFlags stage
) const {
    VkDescriptorSetLayoutBinding b = {};
    b.binding = binding;
    b.descriptorType = type;
    b.descriptorCount = 1;
    b.stageFlags = stage;
    layout.push_back (b);
}

VkDescriptorSetLayout DescriptorSets::createLayout (
    const std::vector<VkDescriptorSetLayoutBinding> &bindings
) {
    VkDescriptorSetLayout layout;
    VkDescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size ());
    createInfo.pBindings = bindings.data ();
    auto result = vkCreateDescriptorSetLayout (device, &createInfo, nullptr, &layout);
    CHECK_VK (result, "Could not create descriptor set layout");
    return layout;
}

}
