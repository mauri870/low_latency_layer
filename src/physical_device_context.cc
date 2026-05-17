#include "physical_device_context.hh"
#include "helper.hh"

#include <vulkan/vulkan_core.h>

#include <ranges>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace low_latency {

static auto
query_supported_extensions(const PhysicalDeviceContext& context) {
    auto count = std::uint32_t{};
    THROW_NOT_VKSUCCESS(
        context.instance.vtable.EnumerateDeviceExtensionProperties(
            context.physical_device, nullptr, &count, nullptr));

    auto supported_extensions = std::vector<VkExtensionProperties>(count);
    THROW_NOT_VKSUCCESS(
        context.instance.vtable.EnumerateDeviceExtensionProperties(
            context.physical_device, nullptr, &count,
            std::data(supported_extensions)));

    return supported_extensions |
           std::views::transform(
               [](const auto& ext) -> std::string_view {
                   return ext.extensionName;
               }) |
           std::ranges::to<std::unordered_set<std::string_view>>();
}

// Returns VK_KHR_calibrated_timestamps if supported, VK_EXT_calibrated_timestamps
// if only the older EXT variant is available, or nullptr if neither is present.
static const char*
pick_calibrated_timestamps_extension(
    const std::unordered_set<std::string_view>& supported) {
    if (supported.contains(VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME)) {
        return VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME;
    }
    if (supported.contains(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME)) {
        return VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME;
    }
    return nullptr;
}

struct ExtensionSupport {
    bool supports_required{};
    const char* calibrated_timestamps_extension{};
};

static ExtensionSupport
check_extension_support(const PhysicalDeviceContext& context) {
    const auto supported = query_supported_extensions(context);
    const auto calibrated =
        pick_calibrated_timestamps_extension(supported);
    const auto supports_required =
        calibrated != nullptr &&
        std::ranges::all_of(
            PhysicalDeviceContext::required_extensions_fixed,
            [&](const auto& ext) { return supported.contains(ext); });
    return {supports_required, calibrated};
}

PhysicalDeviceContext::PhysicalDeviceContext(
    InstanceContext& instance_context, const VkPhysicalDevice& physical_device)
    : instance(instance_context), physical_device(physical_device),
      supports_required_extensions(
          check_extension_support(*this).supports_required),
      calibrated_timestamps_extension(
          check_extension_support(*this).calibrated_timestamps_extension) {

    const auto& vtable = instance_context.vtable;

    this->properties = [&]() {
        auto props = VkPhysicalDeviceProperties{};
        vtable.GetPhysicalDeviceProperties(physical_device, &props);
        return std::make_unique<VkPhysicalDeviceProperties>(std::move(props));
    }();

    this->queue_properties = [&]() {
        auto count = std::uint32_t{};
        vtable.GetPhysicalDeviceQueueFamilyProperties(physical_device, &count,
                                                      nullptr);

        auto result = std::vector<VkQueueFamilyProperties>(
            count, VkQueueFamilyProperties{});
        vtable.GetPhysicalDeviceQueueFamilyProperties(physical_device, &count,
                                                      std::data(result));
        return std::make_unique<std::vector<VkQueueFamilyProperties>>(
            std::move(result));
    }();
}

PhysicalDeviceContext::~PhysicalDeviceContext() {}

} // namespace low_latency