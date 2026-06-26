#ifndef URL_FRONTIER_H
#define URL_FRONTIER_H

#include <string>
#include <queue>
#include <unordered_set>
#include <atomic>
#include <vector>
#include <memory>
#include "crawler_config.h"
#include "metrics_collector.h"
#include "instrumented_mutex.h"


/**
 * URL frontier with minimal locking
 * Uses a single mutex ONLY for the queue operations (dequeue/enqueue)
 * This is necessary for std::queue thread-safety
 */
class URLFrontier {
public:
    URLFrontier();

    /**
     * Initialize frontier with seed URL
     * @param seed_url Starting URL
     * @param config Config pointer
     * @param metrics Metrics collector pointer
     */
    void init(const std::string& seed_url, CrawlerConfig* config = nullptr, MetricsCollector* metrics = nullptr);

    /**
     * Initialize frontier with multiple seed URLs
     * @param seed_urls Vector of starting URLs
     * @param config Config pointer
     * @param metrics Metrics collector pointer
     */
    void init(const std::vector<std::string>& seed_urls, CrawlerConfig* config = nullptr, MetricsCollector* metrics = nullptr);
    
    /**
     * Try to dequeue next URL to crawl
     * @param url Output parameter for dequeued URL
     * @return true if URL was dequeued, false if queue empty
     */
    bool try_dequeue(std::string& url);
    
    /**
     * Add URL if not visited
     * @param url URL to add
     * @return true if added, false if already visited
     */
    bool add_if_not_visited(const std::string& url);
    
    /**
     * Check if has work available
     * @return true if there are URLs to process
     */
    bool has_work() const;
    
    /**
     * Get current queue size (for stats)
     * @return Number of URLs in queue
     */
    size_t queue_size() const;
    
    /**
     * Get number of visited URLs (for stats)
     * @return Number of visited URLs
     */
    size_t visited_count() const;
    
    /**
     * Signal that crawling is complete
     */
    void mark_done();

    /**
     * Create a copy of the internal queue (for read-only dumping).
     * NOTE: Callers should ensure no concurrent modifications (safe after crawl complete).
     */
    std::queue<std::string> copy_queue() const;

    /**
     * Create a copy of the visited set (read-only).
     */
    std::unordered_set<std::string> copy_visited() const; 

    /**
     * Snapshot helpers for debugging/dumping (return copies)
     * These are safe to call and will copy internal structures for
     * evaluator-friendly output. No mutation is performed.
     */
    std::queue<std::string> get_queue_snapshot();
    std::unordered_set<std::string> get_visited_snapshot();
    
    /**
     * Batch enqueue multiple URLs (called from parser)
     * @param urls Vector of URLs to enqueue
     * @return Number of URLs actually added
     */
    int batch_enqueue(const std::vector<std::string>& urls);

private:
    std::queue<std::string> to_visit;
    std::unordered_set<std::string> visited;
    std::vector<std::string> visited_list_; // For naive O(n) scan
    std::atomic<bool> is_done{false};
    std::atomic<size_t> queue_size_{0};
    
    CrawlerConfig* config_{nullptr};
    MetricsCollector* metrics_{nullptr};
    
    // Minimal lock - ONLY protects the queue operations
    mutable InstrumentedMutex queue_mutex;
};

#endif // URL_FRONTIER_H
