#include "benchmark_runner.h"
#include "storage_manager.h"
#include "thread_manager.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

BenchmarkRunner::BenchmarkRunner(const std::string& seed_url, int max_pages, int num_trials)
    : seed_url_(seed_url), max_pages_(max_pages), num_trials_(num_trials) {}

MetricsCollector::Results BenchmarkRunner::run_single(CrawlerConfig config) {
    MetricsCollector metrics;
    metrics.start_total_timer();
    
    StorageManager storage;
    storage.init(config.num_threads, &config, &metrics);
    
    ThreadManager crawler;
    crawler.start(config.num_threads, config.max_pages, config.seed_url, storage, &config, &metrics);
    crawler.wait_completion();
    
    storage.merge_all_buffers();
    storage.compute_pagerank(30);
    
    metrics.stop_total_timer();
    metrics.capture_process_cpu_times();
    metrics.capture_peak_memory();
    
    return metrics.compute_results(config.scenario_name, config.num_threads);
}

AggregatedResults BenchmarkRunner::run_scenario(CrawlerConfig config) {
    std::vector<MetricsCollector::Results> trials;
    std::cout << "\n========================================================" << std::endl;
    std::cout << ">>> Running Scenario: " << config.scenario_name 
              << " (" << num_trials_ << " trials)" << std::endl;
    std::cout << "========================================================" << std::endl;
    
    for (int t = 0; t < num_trials_; ++t) {
        std::cout << "  Trial " << (t + 1) << "/" << num_trials_ << "..." << std::endl;
        trials.push_back(run_single(config));
    }
    return AggregatedResults::from_trials(trials);
}

void BenchmarkRunner::benchmark_multithreading() {
    // A: Single-threaded (multithreading disabled, 1 thread)
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.enable_multithreading = false;
    config_a.num_threads = 1;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "singlethread";
    all_results_.push_back(run_scenario(config_a));

    // B: Multithreaded (multithreading enabled, 4 threads)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.enable_multithreading = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "multithread_4t";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_duplicate_detection() {
    // A: Naive duplicate detection (O(N) vector scan)
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.use_optimized_duplicate_detection = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "dedup_naive";
    all_results_.push_back(run_scenario(config_a));

    // B: Optimized duplicate detection (O(1) unordered_set lookup)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.use_optimized_duplicate_detection = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "dedup_optimized";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_graph_storage() {
    // A: Naive graph storage (Flat edge list)
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.use_optimized_graph_storage = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "graph_naive";
    all_results_.push_back(run_scenario(config_a));

    // B: Optimized graph storage (Adjacency-list)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.use_optimized_graph_storage = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "graph_optimized";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_pagerank() {
    // A: Naive PageRank (map, recomputes out-degrees, no dangling mass redistribution)
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.use_optimized_pagerank = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "pagerank_naive";
    all_results_.push_back(run_scenario(config_a));

    // B: Optimized PageRank (unordered_map, dangling mass, normalization)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.use_optimized_pagerank = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "pagerank_optimized";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_tls_vs_shared() {
    // A: Shared buffer storage with global lock
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.enable_thread_local_storage = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "shared_storage";
    all_results_.push_back(run_scenario(config_a));

    // B: Thread-local storage buffers (Lock-Free addition)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.enable_thread_local_storage = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "tls_storage";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_url_normalization() {
    // A: URL normalization off
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.enable_url_normalization = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "norm_off";
    all_results_.push_back(run_scenario(config_a));

    // B: URL normalization on
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.enable_url_normalization = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "norm_on";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_lock_strategy() {
    // A: Lock heavy strategy (global mutex on all storage actions)
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.enable_lock_minimized = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "lock_heavy";
    all_results_.push_back(run_scenario(config_a));

    // B: Lock minimized strategy (queue-only mutex)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.enable_lock_minimized = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "lock_minimized";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_concurrent_download() {
    // A: Sequential download (global mutex on HTTP requests)
    CrawlerConfig config_a = CrawlerConfig::default_optimized();
    config_a.enable_concurrent_download = false;
    config_a.num_threads = 4;
    config_a.seed_url = seed_url_;
    config_a.max_pages = max_pages_;
    config_a.scenario_name = "download_sequential";
    all_results_.push_back(run_scenario(config_a));

    // B: Concurrent download (parallel HTTP requests)
    CrawlerConfig config_b = CrawlerConfig::default_optimized();
    config_b.enable_concurrent_download = true;
    config_b.num_threads = 4;
    config_b.seed_url = seed_url_;
    config_b.max_pages = max_pages_;
    config_b.scenario_name = "download_concurrent";
    all_results_.push_back(run_scenario(config_b));
}

void BenchmarkRunner::benchmark_scalability() {
    std::vector<int> thread_counts = {1, 2, 4, 8, 16};
    
    // Store baseline average crawl time (1 thread) to calculate speedups
    double time_1t_avg = 0.0;
    
    for (int threads : thread_counts) {
        CrawlerConfig config = CrawlerConfig::default_optimized();
        config.num_threads = threads;
        config.seed_url = seed_url_;
        config.max_pages = max_pages_;
        config.scenario_name = "scale_" + std::to_string(threads) + "t";
        
        AggregatedResults agg = run_scenario(config);
        
        if (threads == 1) {
            time_1t_avg = agg.crawl_time_ms.average;
        }
        
        all_results_.push_back(agg);
    }
    
    // Calculate speedup factor and parallel efficiency for scalability entries
    if (time_1t_avg > 0.0) {
        for (auto& res : all_results_) {
            if (res.scenario_name.rfind("scale_", 0) == 0) {
                res.speedup_factor.average = time_1t_avg / res.crawl_time_ms.average;
                res.speedup_factor.median = time_1t_avg / res.crawl_time_ms.median;
                res.speedup_factor.stddev = 0.0; // stddev not easily combinable here
                
                res.parallel_efficiency.average = res.speedup_factor.average / res.num_threads;
                res.parallel_efficiency.median = res.speedup_factor.median / res.num_threads;
                res.parallel_efficiency.stddev = 0.0;
            }
        }
    }
}

void BenchmarkRunner::run_all() {
    std::cout << "\n=============================================" << std::endl;
    std::cout << "      STARTING AUTOMATED CRAWLER BENCHMARKS  " << std::endl;
    std::cout << "=============================================" << std::endl;
    
    benchmark_multithreading();
    benchmark_duplicate_detection();
    benchmark_graph_storage();
    benchmark_pagerank();
    benchmark_tls_vs_shared();
    benchmark_url_normalization();
    benchmark_lock_strategy();
    benchmark_concurrent_download();
    benchmark_scalability();
}

void BenchmarkRunner::export_results(const std::string& filepath) {
    AggregatedResults::export_csv(all_results_, filepath);
    std::cout << "\n[INFO] Benchmark results exported to: " << filepath << std::endl;
}

void BenchmarkRunner::print_summary() {
    std::cout << "\n==========================================================================================" << std::endl;
    std::cout << "                                  BENCHMARK SUMMARY REPORT                                " << std::endl;
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "  " << std::left << std::setw(22) << "Scenario" 
              << std::right << std::setw(8) << "Threads"
              << std::setw(12) << "Wall(avg)ms"
              << std::setw(12) << "Crawl(avg)ms"
              << std::setw(12) << "PeakMem(MB)"
              << std::setw(10) << "Q_Wait(ms)"
              << std::setw(10) << "Store(ms)"
              << std::setw(10) << "NetWait(ms)" << std::endl;
    std::cout << "------------------------------------------------------------------------------------------" << std::endl;
    
    for (const auto& r : all_results_) {
        std::cout << "  " << std::left << std::setw(22) << r.scenario_name 
                  << std::right << std::setw(8) << r.num_threads
                  << std::fixed << std::setprecision(1)
                  << std::setw(12) << r.wall_clock_time_ms.average
                  << std::setw(12) << r.crawl_time_ms.average
                  << std::setw(12) << r.peak_memory_mb.average
                  << std::setw(10) << r.total_queue_contention_ms.average
                  << std::setw(10) << r.total_storage_mutex_wait_ms.average
                  << std::setw(10) << r.total_network_wait_ms.average << std::endl;
    }
    
    std::cout << "==========================================================================================" << std::endl;
    
    // Print scalability detail
    std::cout << "\n=============================================" << std::endl;
    std::cout << "            SCALABILITY & SPEEDUP            " << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "  " << std::left << std::setw(12) << "Scenario" 
              << std::right << std::setw(10) << "Crawl(avg)ms"
              << std::setw(10) << "Speedup"
              << std::setw(12) << "Efficiency" << std::endl;
    std::cout << "---------------------------------------------" << std::endl;
    
    for (const auto& r : all_results_) {
        if (r.scenario_name.rfind("scale_", 0) == 0) {
            std::cout << "  " << std::left << std::setw(12) << r.scenario_name 
                      << std::right << std::fixed << std::setprecision(2)
                      << std::setw(10) << r.crawl_time_ms.average
                      << std::setw(10) << r.speedup_factor.average
                      << std::setw(12) << r.parallel_efficiency.average << std::endl;
        }
    }
    std::cout << "=============================================" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string seed_url = "http://localhost:8080";
    int max_pages = 50;
    int num_trials = 5;
    
    if (argc >= 2) seed_url = argv[1];
    if (argc >= 3) max_pages = std::stoi(argv[2]);
    if (argc >= 4) num_trials = std::stoi(argv[3]);
    
    std::cout << "Starting benchmarks against seed: " << seed_url 
              << ", max pages: " << max_pages 
              << ", trials: " << num_trials << std::endl;
              
    BenchmarkRunner runner(seed_url, max_pages, num_trials);
    runner.run_all();
    runner.print_summary();
    runner.export_results("benchmark_results.csv");
    
    return 0;
}
