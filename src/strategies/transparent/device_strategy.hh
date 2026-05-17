#ifndef STRATEGIES_TRANSPARENT_DEVICE_STRATEGY_HH_
#define STRATEGIES_TRANSPARENT_DEVICE_STRATEGY_HH_

#include "delay_controller.hh"
#include "strategies/device_strategy.hh"
#include "submission_span.hh"

#include <memory>
#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>

namespace low_latency {

class DeviceContext;

// Transparent latency reduction for games with no anti-lag/reflex support.
// At pre_present time we wait for the previous frame's GPU work to complete,
// capping CPU-ahead-of-GPU depth to ~1 frame (same as driver-level Anti-Lag 1).
class TransparentDeviceStrategy final : public DeviceStrategy {
  private:
    std::mutex mutex{};
    std::vector<std::unique_ptr<SubmissionSpan>> previous_frame_spans{};
    DelayController delay_controller;

  public:
    explicit TransparentDeviceStrategy(DeviceContext& device);
    virtual ~TransparentDeviceStrategy();

  public:
    virtual void
    notify_create_swapchain(const VkSwapchainKHR& swapchain,
                            const VkSwapchainCreateInfoKHR& info) override;
    virtual void
    notify_destroy_swapchain(const VkSwapchainKHR& swapchain) override;

  public:
    // Called before vkQueuePresentKHR: waits for previous frame spans + delays.
    void pre_present();
    // Called after vkQueuePresentKHR: grabs current spans from all queues.
    void on_present_done();
};

} // namespace low_latency

#endif
