#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace Rendering {

enum class Set : uint8_t {
  Dynamic,
  Text,
  Level,
  Deferred,
  Count
};

class DescriptorSets {
 public:
  DescriptorSets(VkDevice device);
  ~DescriptorSets();
  void allocate(VkDescriptorPool pool);
  void update(Set set, uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo info) const;
  void update(Set set, uint32_t binding, VkDescriptorImageInfo info) const;
  VkDescriptorSet set(Set s) const;
  VkDescriptorSetLayout layout(Set s) const;
  const std::unordered_map<VkDescriptorType, uint32_t> &requirements() const;

 private:
  VkDevice device;
  std::unordered_map<VkDescriptorType, uint32_t> req;
  std::vector<VkDescriptorSetLayout> layouts;
  std::vector<VkDescriptorSet> descriptorSets;

  void createLayouts();
  VkDescriptorSetLayout createLayout(const std::vector<VkDescriptorSetLayoutBinding> &layoutBindings);
  void addLayout (
      std::vector<VkDescriptorSetLayoutBinding> &layout,
      uint32_t binding,
      VkDescriptorType type,
      VkShaderStageFlags stage
  ) const;
};

}
