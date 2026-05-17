#include "queue_strategy.hh"
#include "device_context.hh"
#include "device_strategy.hh"
#include "queue_context.hh"

namespace low_latency {

TransparentQueueStrategy::TransparentQueueStrategy(QueueContext& queue)
    : QueueStrategy(queue) {}

TransparentQueueStrategy::~TransparentQueueStrategy() {}

void TransparentQueueStrategy::notify_submit(
    [[maybe_unused]] const VkSubmitInfo& submit,
    std::shared_ptr<TimestampPool::Handle> handle) {

    if (!this->should_track_submissions()) {
        return;
    }

    const auto lock = std::scoped_lock{this->mutex};
    if (this->submission_span) {
        this->submission_span->update(std::move(handle));
    } else {
        this->submission_span =
            std::make_unique<SubmissionSpan>(std::move(handle));
    }
}

void TransparentQueueStrategy::notify_submit(
    [[maybe_unused]] const VkSubmitInfo2& submit,
    std::shared_ptr<TimestampPool::Handle> handle) {

    if (!this->should_track_submissions()) {
        return;
    }

    const auto lock = std::scoped_lock{this->mutex};
    if (this->submission_span) {
        this->submission_span->update(std::move(handle));
    } else {
        this->submission_span =
            std::make_unique<SubmissionSpan>(std::move(handle));
    }
}

void TransparentQueueStrategy::pre_present(const VkPresentInfoKHR&) {
    const auto device_strategy = dynamic_cast<TransparentDeviceStrategy*>(
        this->queue.device.strategy.get());
    assert(device_strategy);
    device_strategy->pre_present();
}

void TransparentQueueStrategy::notify_present(const VkPresentInfoKHR&) {
    const auto device_strategy = dynamic_cast<TransparentDeviceStrategy*>(
        this->queue.device.strategy.get());
    assert(device_strategy);
    device_strategy->on_present_done();
}

bool TransparentQueueStrategy::should_track_submissions() const {
    return this->queue.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

} // namespace low_latency
