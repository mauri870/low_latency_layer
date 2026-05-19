#ifndef PHYSICAL_DEVICE_CONTEXT_HH_
#define PHYSICAL_DEVICE_CONTEXT_HH_

#include "instance_context.hh"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "context.hh"

namespace low_latency {

class PhysicalDeviceContext final : public Context {
  public:
    // Extensions we need for our layer to function.
    // If the PD doesn't support this then the layer shouldn't set the anti lag
    // flag in VkGetPhysicalDevices2 (check this->supports_required_extensions).
    static constexpr auto required_extensions_fixed = {
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME};

  public:
    InstanceContext& instance;
    const VkPhysicalDevice physical_device{};
    const bool supports_required_extensions{};
    // VK_KHR_calibrated_timestamps if supported; falls back to the equivalent
    // VK_EXT_calibrated_timestamps on drivers that predate the KHR promotion.
    const char* const calibrated_timestamps_extension{};

    std::unique_ptr<VkPhysicalDeviceProperties> properties{};
    std::unique_ptr<std::vector<VkQueueFamilyProperties>> queue_properties{};

  public:
    explicit PhysicalDeviceContext(InstanceContext& instance_context,
                                   const VkPhysicalDevice& physical_device);
    virtual ~PhysicalDeviceContext();
};

} // namespace low_latency

#endif