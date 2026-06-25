#ifndef BENCHMARK_RUNNER_H
#define BENCHMARK_RUNNER_H

#include "crawler_config.h"
#include "metrics_collector.h"
#include <string>
#include <vector>

class BenchmarkRunner {
public:
    BenchmarkRunner(const std::string& seed_url, int max_pages, int num_trials = 5);

    void run_all();

    // === Individual subsystem benchmarks ===
    void benchmark_multithreading();
    void benchmark_duplicate_detection();
    void benchmark_graph_storage();
    void benchmark_pagerank();
    void benchmark_tls_vs_shared();
    void benchmark_url_normalization();
    void benchmark_lock_strategy();
    void benchmark_concurrent_download();
    void benchmark_scalability();

    // === Export ===
    void export_results(const std::string& filepath = "benchmark_results.csv");
    void print_summary();

private:
    AggregatedResults run_scenario(CrawlerConfig config);
    MetricsCollector::Results run_single(CrawlerConfig config);

    std::vector<AggregatedResults> all_results_;
    std::string seed_url_;
    int max_pages_;
    int num_trials_;
};

#endif // BENCHMARK_RUNNER_H
