#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <thread_manager.h>
#include <storage_manager.h>

void print_usage(const char* program_name) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Multithreaded Web Crawler (Lock-Free)           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nUsage: " << program_name << " <seed_url> <max_pages> <num_threads>" << std::endl;
    std::cout << "\nArguments:" << std::endl;
    std::cout << "  seed_url     - Starting URL (e.g., https://example.com)" << std::endl;
    std::cout << "  max_pages    - Maximum number of pages to crawl (e.g., 100)" << std::endl;
    std::cout << "  num_threads  - Number of worker threads (e.g., 4)" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << program_name << " https://example.com 100 4" << std::endl;
    std::cout << "\nOutput:" << std::endl;
    std::cout << "  crawled_pages.csv     - Pages crawled with link counts" << std::endl;
    std::cout << "  pagerank_results.csv  - PageRank scores for each domain" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string seed_url = argv[1];
    std::string max_pages_str = argv[2];
    std::string num_threads_str = argv[3];
    
    // Validate inputs
    if (seed_url.find("http://") != 0 && seed_url.find("https://") != 0) {
        std::cerr << "[ERROR] Seed URL must start with http:// or https://" << std::endl;
        return 1;
    }
    
    int max_pages = 0;
    int num_threads = 0;
    
    try {
        max_pages = std::stoi(max_pages_str);
        num_threads = std::stoi(num_threads_str);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Invalid arguments: " << e.what() << std::endl;
        return 1;
    }
    
    if (max_pages <= 0) {
        std::cerr << "[ERROR] max_pages must be positive" << std::endl;
        return 1;
    }
    
    if (num_threads <= 0) {
        std::cerr << "[ERROR] num_threads must be positive" << std::endl;
        return 1;
    }
    
    if (num_threads > 64) {
        std::cerr << "[ERROR] num_threads cannot exceed 64" << std::endl;
        return 1;
    }



    
    // Initialize storage
    StorageManager storage;
    storage.init(num_threads);
    
    // Crawling - measure time
    std::cout << "\n[TIMING] Starting crawling..." << std::endl;
    auto crawl_start = std::chrono::high_resolution_clock::now();
    
    // Start crawling
    ThreadManager crawler;
    crawler.start(num_threads, max_pages, seed_url, storage);
    
    // Wait for all threads to complete
    crawler.wait_completion();

#if DEBUG_DUMP
    // Dump frontier and visited snapshot for debugging
    crawler.dump_frontier_and_visited();
#endif
    
    auto crawl_end = std::chrono::high_resolution_clock::now();
    auto crawl_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        crawl_end - crawl_start);
    std::cout << "[TIMING] Crawling completed in " << std::fixed << std::setprecision(3)
              << crawl_duration.count() << " ms" << std::endl;
    
    // Domain counting - measure time
    std::cout << "\n[TIMING] Starting domain counting..." << std::endl;
    auto domain_count_start = std::chrono::high_resolution_clock::now();
    
    storage.merge_all_buffers();
    
    auto domain_count_end = std::chrono::high_resolution_clock::now();
    auto domain_count_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        domain_count_end - domain_count_start);
    std::cout << "[TIMING] Domain counting completed in " << std::fixed << std::setprecision(3)
              << domain_count_duration.count() << " ms" << std::endl;
    
    // PageRank computation - measure time
    std::cout << "\n[TIMING] Starting PageRank computation..." << std::endl;
    auto pagerank_start = std::chrono::high_resolution_clock::now();
    
    storage.compute_pagerank(30);
    
    auto pagerank_end = std::chrono::high_resolution_clock::now();
    auto pagerank_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        pagerank_end - pagerank_start);
    std::cout << "[TIMING] PageRank computation completed in " << std::fixed << std::setprecision(3)
              << pagerank_duration.count() << " ms" << std::endl;
    
    // Export results
    storage.export_to_csv("crawled_pages.csv", "pagerank_results.csv");
    
    // Log metrics to CSV
    int pages_crawled = crawler.get_pages_crawled();
    long long total_ms = crawl_duration.count();
    double throughput = (total_ms > 0) ? pages_crawled * 1000.0 / total_ms : 0.0;
    
    std::ofstream metrics_out("metrics.csv", std::ios::app);
    if (!metrics_out.is_open()) {
        std::cerr << "[ERROR] Could not open metrics.csv for writing" << std::endl;
    } else {
        // Check if file is empty (new file) to write header
        metrics_out.seekp(0, std::ios::end);
        if (metrics_out.tellp() == 0) {
            metrics_out << "seed_url,max_pages,num_threads,total_ms,pages_crawled,throughput\n";
        }
        metrics_out << seed_url << ","
                    << max_pages << ","
                    << num_threads << ","
                    << total_ms << ","
                    << pages_crawled << ","
                    << std::fixed << std::setprecision(2) << throughput << "\n";
        metrics_out.close();
        std::cout << "[INFO] Metrics appended to: metrics.csv" << std::endl;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    CRAWL FINISHED                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n[RESULTS]" << std::endl;
    std::cout << "  Pages crawled:   " << crawler.get_pages_crawled() << std::endl;
    std::cout << "  CSV files generated:" << std::endl;
    std::cout << "    - crawled_pages.csv" << std::endl;
    std::cout << "    - pagerank_results.csv" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
