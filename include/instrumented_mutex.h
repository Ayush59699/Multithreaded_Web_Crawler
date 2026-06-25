#ifndef INSTRUMENTED_MUTEX_H
#define INSTRUMENTED_MUTEX_H

#include "metrics_collector.h"
#include <mutex>
#include <chrono>

class InstrumentedMutex {
public:
    enum class Category { QUEUE, STORAGE };

    InstrumentedMutex(MetricsCollector* metrics, Category cat)
        : metrics_(metrics), category_(cat) {}

    void set_metrics(MetricsCollector* metrics) {
        metrics_ = metrics;
    }

    void lock() {
        auto t0 = std::chrono::high_resolution_clock::now();
        inner_.lock();
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (metrics_) {
            if (category_ == Category::QUEUE) {
                metrics_->record_queue_contention_time(ms);
            } else {
                metrics_->record_storage_mutex_wait_time(ms);
            }
        }
    }

    void unlock() {
        inner_.unlock();
    }

    bool try_lock() {
        // try_lock doesn't block, so contention wait time is 0
        return inner_.try_lock();
    }

private:
    std::mutex inner_;
    MetricsCollector* metrics_;
    Category category_;
};

// RAII guard compatible with std::lock_guard
class InstrumentedLockGuard {
public:
    explicit InstrumentedLockGuard(InstrumentedMutex& mtx) : mtx_(mtx) {
        mtx_.lock();
    }
    ~InstrumentedLockGuard() {
        mtx_.unlock();
    }
    InstrumentedLockGuard(const InstrumentedLockGuard&) = delete;
    InstrumentedLockGuard& operator=(const InstrumentedLockGuard&) = delete;

private:
    InstrumentedMutex& mtx_;
};

#endif // INSTRUMENTED_MUTEX_H
