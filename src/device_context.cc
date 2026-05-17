#include "device_context.hh"

#include "layer_context.hh"
#include "strategies/anti_lag/device_strategy.hh"
#include "strategies/low_latency2/device_strategy.hh"
#include "strategies/transparent/device_strategy.hh"

#include <utility>
#include <vulkan/vulkan_core.h>

namespace low_latency {

DeviceContext::DeviceContext(InstanceContext& parent_instance,
                             PhysicalDeviceContext& parent_physical_device,
                             const VkDevice& device,
                             const bool was_layer_enabled,
                             const bool is_transparent_active,
                             VkuDeviceDispatchTable&& vtable)
    : instance(parent_instance), physical_device(parent_physical_device),
      was_layer_enabled(was_layer_enabled),
      is_transparent_active(is_transparent_active), device(device),
      vtable(std::move(vtable)) {

    if (!this->was_layer_enabled && !this->is_transparent_active) {
        return;
    }

    this->clock = std::make_unique<DeviceClock>(*this);
    this->strategy = [&]() -> std::unique_ptr<DeviceStrategy> {
        if (!this->was_layer_enabled) {
            return std::make_unique<TransparentDeviceStrategy>(*this);
        }
        if (parent_instance.layer.should_expose_reflex) {
            return std::make_unique<LowLatency2DeviceStrategy>(*this);
        }
        return std::make_unique<AntiLagDeviceStrategy>(*this);
    }();
}

DeviceContext::~DeviceContext() {}

} // namespace low_latency