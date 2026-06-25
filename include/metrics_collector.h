#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include <string>
#include <vector>
#include <atomic>
#include <chrono>

class MetricsCollector {
public:
    struct Results {
        std::string scenario_name;
        int num_threads = 0;

        // Timing (ms)
        double wall_clock_time_ms = 0.0;
        double crawl_time_ms = 0.0;
        double graph_construction_time_ms = 0.0;
        double pagerank_time_ms = 0.0;
        double user_cpu_time_ms = 0.0;     // Process user-mode CPU time
        double system_cpu_time_ms = 0.0;   // Process kernel-mode CPU time

        // Throughput
        double pages_per_sec = 0.0;
        double urls_per_sec = 0.0;
        double graph_edges_per_sec = 0.0;
        double pagerank_nodes_per_sec = 0.0;

        // Scalability (computed against a baseline)
        double speedup_factor = 1.0;
        double parallel_efficiency = 1.0;

        // Counts
        int pages_crawled = 0;
        int urls_processed = 0;
        int urls_enqueued = 0;
        int duplicate_urls_rejected = 0;
        int graph_edges_inserted = 0;
        int pagerank_nodes = 0;
        int requests_failed = 0;

        // Efficiency
        double duplicate_elimination_pct = 0.0;

        // Network
        double total_network_wait_ms = 0.0;
        double network_idle_pct = 0.0;

        // Lock contention
        double total_queue_contention_ms = 0.0;
        double total_storage_mutex_wait_ms = 0.0;

        // Memory
        double peak_memory_mb = 0.0;
        double memory_per_page_kb = 0.0;
    };

    MetricsCollector() = default;

    // === Timers ===
    void start_total_timer();
    void stop_total_timer();
    void start_crawl_timer();
    void stop_crawl_timer();
    void start_graph_timer();
    void stop_graph_timer();
    void start_pagerank_timer();
    void stop_pagerank_timer();

    // === Counters (thread-safe via std::atomic) ===
    void record_page_crawled();
    void record_url_processed();
    void record_duplicate_url_rejected();
    void record_url_enqueued();
    void record_graph_edge_inserted();
    void record_download_failed();
    void record_pagerank_nodes(int count);


    // === Timing accumulators (thread-safe) ===
    void record_network_wait_time(double ms);
    void record_queue_contention_time(double ms);   // URLFrontier mutex waits
    void record_storage_mutex_wait_time(double ms);  // StorageManager mutex waits

    // === System metrics (sampled at end) ===
    void capture_process_cpu_times();  // user + kernel CPU time
    void capture_peak_memory();

    // === Results ===
    Results compute_results(const std::string& scenario_name, int num_threads) const;
    void reset();

private:
    // Timestamps
    std::chrono::high_resolution_clock::time_point total_start_;
    std::chrono::high_resolution_clock::time_point total_end_;
    std::chrono::high_resolution_clock::time_point crawl_start_;
    std::chrono::high_resolution_clock::time_point crawl_end_;
    std::chrono::high_resolution_clock::time_point graph_start_;
    std::chrono::high_resolution_clock::time_point graph_end_;
    std::chrono::high_resolution_clock::time_point pagerank_start_;
    std::chrono::high_resolution_clock::time_point pagerank_end_;

    // Atomic Counters
    std::atomic<int> pages_crawled_{0};
    std::atomic<int> urls_processed_{0};
    std::atomic<int> urls_enqueued_{0};
    std::atomic<int> duplicate_urls_rejected_{0};
    std::atomic<int> graph_edges_inserted_{0};
    std::atomic<int> requests_failed_{0};
    std::atomic<int> pagerank_nodes_{0};


    // Timing Accumulators (stored in microseconds to avoid floating point atomic operations)
    std::atomic<int64_t> total_network_wait_us_{0};
    std::atomic<int64_t> total_queue_contention_us_{0};
    std::atomic<int64_t> total_storage_mutex_wait_us_{0};

    // System resource values
    double user_cpu_time_ms_{0.0};
    double system_cpu_time_ms_{0.0};
    double peak_memory_mb_{0.0};
};

// === Aggregated results across trials ===
struct AggregatedResults {
    std::string scenario_name;
    int num_threads = 0;
    int num_trials = 0;

    struct Stat {
        double average = 0.0;
        double median = 0.0;
        double stddev = 0.0;
    };

    Stat wall_clock_time_ms;
    Stat crawl_time_ms;
    Stat pages_per_sec;
    Stat urls_per_sec;
    Stat user_cpu_time_ms;
    Stat system_cpu_time_ms;
    Stat speedup_factor;
    Stat parallel_efficiency;
    Stat peak_memory_mb;
    Stat memory_per_page_kb;
    Stat total_network_wait_ms;
    Stat total_queue_contention_ms;
    Stat total_storage_mutex_wait_ms;
    Stat duplicate_elimination_pct;
    Stat graph_construction_time_ms;
    Stat graph_edges_per_sec;
    Stat pagerank_time_ms;
    Stat pagerank_nodes_per_sec;
    Stat requests_failed;

    static AggregatedResults from_trials(
        const std::vector<MetricsCollector::Results>& trials);

    static void export_csv(const std::vector<AggregatedResults>& all,
                           const std::string& filepath);
};

#endif // METRICS_COLLECTOR_H
