#include "url_frontier.h"
#include <chrono>
#include <thread>

void URLFrontier::init(const std::string& seed_url) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    to_visit.push(seed_url);
    visited.insert(seed_url);
    queue_size_.store(1);
    is_done.store(false);
}

bool URLFrontier::try_dequeue(std::string& url) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    
    if (to_visit.empty()) {
        return false;
    }
    
    url = to_visit.front();
    to_visit.pop();
    queue_size_.store(to_visit.size());
    return true;
}

bool URLFrontier::add_if_not_visited(const std::string& url) {
    // Validate URL first (no lock needed)
    if (url.empty() || url.length() > 10000) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex);
    
    // Check if already visited
    if (visited.find(url) != visited.end()) {
        return false;
    }
    
    // Insert and add to queue
    auto result = visited.insert(url);
    if (result.second) {
        to_visit.push(url);
        queue_size_.store(to_visit.size());
        return true;
    }
    
    return false;
}

bool URLFrontier::has_work() const {
    return !to_visit.empty() && !is_done.load();
}

size_t URLFrontier::queue_size() const {
    return queue_size_.load();
}

size_t URLFrontier::visited_count() const {
    return visited.size();
}

void URLFrontier::mark_done() {
    is_done.store(true);
}

std::queue<std::string> URLFrontier::copy_queue() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return to_visit;
}

std::unordered_set<std::string> URLFrontier::copy_visited() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
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
    std::lock_guard<std::mutex> lock(queue_mutex);
    return to_visit; // copy
}

std::unordered_set<std::string> URLFrontier::get_visited_snapshot() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return visited; // copy
}
