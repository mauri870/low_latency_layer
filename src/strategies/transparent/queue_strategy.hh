#ifndef STRATEGIES_TRANSPARENT_QUEUE_STRATEGY_HH_
#define STRATEGIES_TRANSPARENT_QUEUE_STRATEGY_HH_

#include "strategies/queue_strategy.hh"
#include "submission_span.hh"

#include <memory>
#include <mutex>

namespace low_latency {

class QueueContext;

class TransparentQueueStrategy final : public QueueStrategy {
  public:
    std::mutex mutex{};
    std::unique_ptr<SubmissionSpan> submission_span{};

  public:
    explicit TransparentQueueStrategy(QueueContext& queue);
    virtual ~TransparentQueueStrategy();

  public:
    virtual void
    notify_submit(const VkSubmitInfo& submit,
                  std::shared_ptr<TimestampPool::Handle> handle) override;
    virtual void
    notify_submit(const VkSubmitInfo2& submit,
                  std::shared_ptr<TimestampPool::Handle> handle) override;
    virtual void pre_present(const VkPresentInfoKHR& present) override;
    virtual void notify_present(const VkPresentInfoKHR& present) override;

  private:
    bool should_track_submissions() const;
};

} // namespace low_latency

#endif
