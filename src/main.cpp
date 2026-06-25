#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include "thread_manager.h"
#include "storage_manager.h"
#include "crawler_config.h"
#include "metrics_collector.h"

void print_usage(const char* program_name) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Multithreaded Web Crawler (Benchmarked)         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nUsage:" << std::endl;
    std::cout << "  " << program_name << " <seed_url> <max_pages> <num_threads>" << std::endl;
    std::cout << "  " << program_name << " --config <config_json_path>" << std::endl;
    std::cout << "\nArguments:" << std::endl;
    std::cout << "  seed_url     - Starting URL (e.g., http://localhost:8080)" << std::endl;
    std::cout << "  max_pages    - Maximum number of pages to crawl" << std::endl;
    std::cout << "  num_threads  - Number of worker threads" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << program_name << " http://localhost:8080 50 4" << std::endl;
    std::cout << "  " << program_name << " --config benchmark.json" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    CrawlerConfig config;
    bool has_config = false;
    
    if (argc == 3 && std::string(argv[1]) == "--config") {
        config = CrawlerConfig::from_json(argv[2]);
        has_config = true;
    } else if (argc == 4) {
        config.seed_url = argv[1];
        try {
            config.max_pages = std::stoi(argv[2]);
            config.num_threads = std::stoi(argv[3]);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Invalid arguments: " << e.what() << std::endl;
            return 1;
        }
    } else {
        print_usage(argv[0]);
        return 1;
    }
    
    // Validate inputs
    if (config.seed_url.find("http://") != 0 && config.seed_url.find("https://") != 0) {
        std::cerr << "[ERROR] Seed URL must start with http:// or https://" << std::endl;
        return 1;
    }
    if (config.max_pages <= 0) {
        std::cerr << "[ERROR] max_pages must be positive" << std::endl;
        return 1;
    }
    if (config.num_threads <= 0) {
        std::cerr << "[ERROR] num_threads must be positive" << std::endl;
        return 1;
    }
    if (config.num_threads > 64) {
        std::cerr << "[ERROR] num_threads cannot exceed 64" << std::endl;
        return 1;
    }

    config.print();
    
    MetricsCollector metrics;
    metrics.start_total_timer();
    
    // Initialize storage
    StorageManager storage;
    storage.init(config.num_threads, &config, &metrics);
    
    // Start crawling
    ThreadManager crawler;
    crawler.start(config.num_threads, config.max_pages, config.seed_url, storage, &config, &metrics);
    
    // Wait for all threads to complete
    crawler.wait_completion();

#if DEBUG_DUMP
    crawler.dump_frontier_and_visited();
#endif
    
    // Domain counting / merging
    storage.merge_all_buffers();
    
    // PageRank computation
    storage.compute_pagerank(30);
    
    // Export results
    storage.export_to_csv("crawled_pages.csv", "pagerank_results.csv");
    
    metrics.stop_total_timer();
    metrics.capture_process_cpu_times();
    metrics.capture_peak_memory();
    
    MetricsCollector::Results r = metrics.compute_results(config.scenario_name, config.num_threads);
    
    // Print metrics to stdout
    std::cout << "\n=============================================" << std::endl;
    std::cout << "            CRAWL / BENCHMARK METRICS        " << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "  Scenario Name:         " << r.scenario_name << std::endl;
    std::cout << "  Wall clock time:       " << std::fixed << std::setprecision(2) << r.wall_clock_time_ms << " ms" << std::endl;
    std::cout << "  Crawl time:            " << r.crawl_time_ms << " ms" << std::endl;
    std::cout << "  Graph construction:    " << r.graph_construction_time_ms << " ms" << std::endl;
    std::cout << "  PageRank time:         " << r.pagerank_time_ms << " ms" << std::endl;
    std::cout << "  User CPU time:         " << r.user_cpu_time_ms << " ms" << std::endl;
    std::cout << "  System CPU time:       " << r.system_cpu_time_ms << " ms" << std::endl;
    std::cout << "  Peak working set:      " << r.peak_memory_mb << " MB" << std::endl;
    std::cout << "  Memory per page:       " << r.memory_per_page_kb << " KB" << std::endl;
    std::cout << "  Throughput (pages/s):  " << r.pages_per_sec << std::endl;
    std::cout << "  Throughput (URLs/s):   " << r.urls_per_sec << std::endl;
    std::cout << "  Queue contention:      " << r.total_queue_contention_ms << " ms" << std::endl;
    std::cout << "  Storage mutex wait:    " << r.total_storage_mutex_wait_ms << " ms" << std::endl;
    std::cout << "  Network wait time:     " << r.total_network_wait_ms << " ms" << std::endl;
    std::cout << "  Network idle %:        " << r.network_idle_pct << " %" << std::endl;
    std::cout << "  Duplicate rejects %:   " << r.duplicate_elimination_pct << " %" << std::endl;
    std::cout << "  Requests failed:       " << r.requests_failed << std::endl;
    std::cout << "  PageRank node count:   " << r.pagerank_nodes << std::endl;
    std::cout << "=============================================" << std::endl;

    // Log metrics to CSV
    std::ofstream metrics_out("metrics.csv", std::ios::app);
    if (!metrics_out.is_open()) {
        std::cerr << "[ERROR] Could not open metrics.csv for writing" << std::endl;
    } else {
        metrics_out.seekp(0, std::ios::end);
        if (metrics_out.tellp() == 0) {
            metrics_out << "seed_url,max_pages,num_threads,total_ms,pages_crawled,throughput\n";
        }
        metrics_out << config.seed_url << ","
                    << config.max_pages << ","
                    << config.num_threads << ","
                    << static_cast<long long>(r.wall_clock_time_ms) << ","
                    << r.pages_crawled << ","
                    << std::fixed << std::setprecision(2) << r.pages_per_sec << "\n";
        metrics_out.close();
        std::cout << "[INFO] Metrics appended to: metrics.csv" << std::endl;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "\n[RESULTS]" << std::endl;
    return 0;
}
