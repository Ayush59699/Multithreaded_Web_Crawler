#ifndef CRAWLER_CONFIG_H
#define CRAWLER_CONFIG_H

#include <string>

struct CrawlerConfig {
    // === Crawl parameters ===
    std::string seed_url = "http://localhost:8080";
    int max_pages = 50;
    int num_threads = 4;

    // === Subsystem strategy flags ===
    // true = optimized implementation, false = naive implementation

    bool enable_multithreading = true;
    // false -> single-threaded: worker_loop runs on main thread
    // true  -> thread pool with num_threads workers

    bool use_optimized_duplicate_detection = true;
    // false -> O(n) linear scan through std::vector<string>
    // true  -> O(1) lookup via std::unordered_set<string>

    bool use_optimized_graph_storage = true;
    // false -> flat std::vector<pair<string,string>> edge list (linear scan to query)
    // true  -> std::unordered_map<string, vector<string>> adjacency list

    bool use_optimized_pagerank = true;
    // false -> naive: std::map, recompute out-degrees each iteration, no dangling node handling, no normalization
    // true  -> optimized: unordered_map, precomputed out-degrees, dangling mass redistribution, normalization

    bool enable_thread_local_storage = true;
    // false -> all threads write to shared buffer[0] with mutex
    // true  -> each thread writes to its own buffer (no locks)

    bool enable_url_normalization = true;
    // false -> raw URLs passed through without normalization
    // true  -> fragment removal, lowercasing, whitespace trim

    bool enable_lock_minimized = true;
    // false -> global mutex protects all storage operations
    // true  -> only URL queue uses mutex (current design)

    bool enable_concurrent_download = true;
    // false -> global download mutex: one HTTP request at a time
    // true  -> all threads download in parallel

    // === Benchmark settings ===
    std::string scenario_name = "default";
    int num_trials = 5;

    // === Factory methods ===
    static CrawlerConfig from_json(const std::string& filepath);
    static CrawlerConfig default_optimized();  // All flags = true
    static CrawlerConfig default_naive();      // All flags = false
    void print() const;
};

#endif // CRAWLER_CONFIG_H
