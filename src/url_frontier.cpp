#include "url_frontier.h"
#include <chrono>
#include <thread>

URLFrontier::URLFrontier() : queue_mutex(nullptr, InstrumentedMutex::Category::QUEUE) {}

void URLFrontier::init(const std::string& seed_url, CrawlerConfig* config, MetricsCollector* metrics) {
    std::vector<std::string> seed_urls = { seed_url };
    init(seed_urls, config, metrics);
}

void URLFrontier::init(const std::vector<std::string>& seed_urls, CrawlerConfig* config, MetricsCollector* metrics) {
    config_ = config;
    metrics_ = metrics;
    queue_mutex.set_metrics(metrics);

    InstrumentedLockGuard lock(queue_mutex);
    to_visit = std::queue<std::string>();
    visited.clear();
    visited_list_.clear();

    size_t initial_size = 0;
    for (const auto& url : seed_urls) {
        if (url.empty() || url.length() > 10000) continue;
        to_visit.push(url);
        if (config_ && !config_->use_optimized_duplicate_detection) {
            visited_list_.push_back(url);
        } else {
            visited.insert(url);
        }
        initial_size++;
    }
    queue_size_.store(initial_size);
    is_done.store(false);
}



bool URLFrontier::try_dequeue(std::string& url) {
    InstrumentedLockGuard lock(queue_mutex);
    
    if (to_visit.empty()) {
        return false;
    }
    
    url = to_visit.front();
    to_visit.pop();
    queue_size_.store(to_visit.size());
    return true;
}

bool URLFrontier::add_if_not_visited(const std::string& url) {
    if (url.empty() || url.length() > 10000) {
        return false;
    }
    
    if (metrics_) {
        metrics_->record_url_processed();
    }

    InstrumentedLockGuard lock(queue_mutex);
    
    if (config_ && !config_->use_optimized_duplicate_detection) {
        // Naive duplicate detection - O(N) vector scan
        for (const auto& v : visited_list_) {
            if (v == url) {
                if (metrics_) metrics_->record_duplicate_url_rejected();
                return false;
            }
        }
        visited_list_.push_back(url);
        to_visit.push(url);
        queue_size_.store(to_visit.size());
        if (metrics_) metrics_->record_url_enqueued();
        return true;
    } else {
        // Optimized duplicate detection - O(1) unordered_set lookup
        if (visited.find(url) != visited.end()) {
            if (metrics_) metrics_->record_duplicate_url_rejected();
            return false;
        }
        
        auto result = visited.insert(url);
        if (result.second) {
            to_visit.push(url);
            queue_size_.store(to_visit.size());
            if (metrics_) metrics_->record_url_enqueued();
            return true;
        }
    }
    
    if (metrics_) metrics_->record_duplicate_url_rejected();
    return false;
}

bool URLFrontier::has_work() const {
    return !to_visit.empty() && !is_done.load();
}

size_t URLFrontier::queue_size() const {
    return queue_size_.load();
}

size_t URLFrontier::visited_count() const {
    InstrumentedLockGuard lock(queue_mutex);
    if (config_ && !config_->use_optimized_duplicate_detection) {
        return visited_list_.size();
    }
    return visited.size();
}

void URLFrontier::mark_done() {
    is_done.store(true);
}

std::queue<std::string> URLFrontier::copy_queue() const {
    InstrumentedLockGuard lock(queue_mutex);
    return to_visit;
}

std::unordered_set<std::string> URLFrontier::copy_visited() const {
    InstrumentedLockGuard lock(queue_mutex);
    if (config_ && !config_->use_optimized_duplicate_detection) {
        return std::unordered_set<std::string>(visited_list_.begin(), visited_list_.end());
    }
    return visited;
}

int URLFrontier::batch_enqueue(const std::vector<std::string>& urls) {
    int added = 0;
    for (const auto& url : urls) {
        if (add_if_not_visited(url)) {
            added++;
        }
    }
    return added;
} 

std::queue<std::string> URLFrontier::get_queue_snapshot() {
    InstrumentedLockGuard lock(queue_mutex);
    return to_visit;
}

std::unordered_set<std::string> URLFrontier::get_visited_snapshot() {
    InstrumentedLockGuard lock(queue_mutex);
    if (config_ && !config_->use_optimized_duplicate_detection) {
        return std::unordered_set<std::string>(visited_list_.begin(), visited_list_.end());
    }
    return visited;
}

