#include "device_strategy.hh"
#include "device_context.hh"
#include "queue_strategy.hh"

#include <algorithm>
#include <ranges>

namespace low_latency {

TransparentDeviceStrategy::TransparentDeviceStrategy(DeviceContext& device)
    : DeviceStrategy(device),
      delay_controller(device.instance.is_simulation_decoupled) {}

TransparentDeviceStrategy::~TransparentDeviceStrategy() {}

void TransparentDeviceStrategy::notify_create_swapchain(
    const VkSwapchainKHR&, const VkSwapchainCreateInfoKHR&) {}

void TransparentDeviceStrategy::notify_destroy_swapchain(
    const VkSwapchainKHR&) {}

void TransparentDeviceStrategy::pre_present() {
    const auto spans = [&]() {
        const auto lock = std::scoped_lock{this->mutex};
        return std::move(this->previous_frame_spans);
    }();

    // Non-blocking check: if any span isn't finished, the GPU is still
    // rendering the previous frame. CPU and GPU are already in lockstep —
    // no pre-render queue has built up, so skip to avoid a GPU pipeline bubble.
    if (spans.empty() ||
        !std::ranges::all_of(spans, [](const auto& span) {
            return !span || span->has_completed();
        })) {
        this->delay_controller.delay(std::chrono::nanoseconds{0});
        return;
    }

    // GPU finished the previous frame before we reached present — the CPU was
    // running ahead. Collect the frame's GPU time boundaries across all queues.
    auto gpu_start = DeviceClock::time_point::max();
    auto gpu_end = DeviceClock::time_point{};
    for (const auto& span : spans) {
        if (!span) {
            continue;
        }
        // await_completed returns immediately since has_completed() was true.
        const auto [start, end] = span->await_completed();
        if (start < gpu_start) {
            gpu_start = start;
        }
        if (end > gpu_end) {
            gpu_end = end;
        }
    }

    // Pace the CPU to the GPU's actual frame time, draining the pre-render
    // queue to ~1 frame without stalling the GPU pipeline.
    this->delay_controller.delay(gpu_end - gpu_start);
}

void TransparentDeviceStrategy::on_present_done() {
    auto new_spans = std::vector<std::unique_ptr<SubmissionSpan>>{};

    const auto device_lock = std::shared_lock{this->device.mutex};
    for (const auto& [_, queue] : this->device.queues) {
        if (!(queue->properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            continue;
        }

        const auto strategy =
            dynamic_cast<TransparentQueueStrategy*>(queue->strategy.get());
        assert(strategy);

        const auto queue_lock = std::scoped_lock{strategy->mutex};
        if (strategy->submission_span) {
            new_spans.push_back(std::move(strategy->submission_span));
            strategy->submission_span.reset();
        }
    }

    const auto lock = std::scoped_lock{this->mutex};
    this->previous_frame_spans = std::move(new_spans);
}

} // namespace low_latency
