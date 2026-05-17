#include "device_strategy.hh"
#include "device_context.hh"
#include "queue_strategy.hh"

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
    // Grab the previous frame's spans under the lock, then release it before
    // the potentially blocking await_completed calls.
    const auto spans = [&]() {
        const auto lock = std::scoped_lock{this->mutex};
        return std::move(this->previous_frame_spans);
    }();

    for (const auto& span : spans) {
        if (span) {
            span->await_completed();
        }
    }

    this->delay_controller.delay(std::chrono::microseconds{0});
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
