#include "thread_manager.h"
#include "DumpUtils.h"
#include <iostream>
#include <chrono>
#include <thread>

void ThreadManager::start(int num_threads, int max_pages,
                          const std::string& seed_url,
                          StorageManager& storage_manager) {
    max_pages_limit.store(max_pages);
    
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║      MULTITHREADED WEB CRAWLER (Lock-Free)            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n[CONFIG]" << std::endl;
    std::cout << "  Seed URL:     " << seed_url << std::endl;
    std::cout << "  Max Pages:    " << max_pages << std::endl;
    std::cout << "  Threads:      " << num_threads << std::endl;
    std::cout << "  Mode:         Lock-Free (No Mutexes)" << std::endl;
    std::cout << "\n[STARTING CRAWL]" << std::endl;
    
    frontier.init(seed_url);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(&ThreadManager::worker_loop, this, i, 
                           std::ref(storage_manager));
    }
    
    // Print progress every second
    std::thread progress_thread([this, num_threads]() {
        while (pages_crawled.load() < max_pages_limit.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            std::cout << "[PROGRESS] Pages: " << pages_crawled.load() 
                      << "/" << max_pages_limit.load()
                      << " | Queue: " << frontier.queue_size()
                      << " | Visited: " << frontier.visited_count() << std::endl;
            
            if (frontier.queue_size() == 0 && pages_crawled.load() > 0) {
                break;
            }
        }
    });
    progress_thread.detach();
}

void ThreadManager::worker_loop(int thread_id, StorageManager& storage_manager) {
    Downloader downloader;
    Parser parser;
    
    std::string url;
    int backoff_ms = 10;
    
    while (pages_crawled.load() < max_pages_limit.load()) {
        // Try to dequeue URL
        if (frontier.try_dequeue(url)) {
            backoff_ms = 10;  // Reset backoff
            
            std::cout << "[T" << thread_id << "] Downloading: " << url << std::endl;
            
            // Download
            std::string html = downloader.download(url);
            
            if (html.empty()) {
                std::cout << "[T" << thread_id << "] ✗ Failed to download: " << url << std::endl;
                continue;
            }
            
            std::string domain = downloader.get_domain(url);
            std::cout << "[T" << thread_id << "] ✓ Downloaded (" << html.size() 
                      << " bytes) from domain: " << domain << std::endl;
            
            // Parse links
            std::vector<std::string> links = parser.extract_links(html, url);
            std::cout << "[T" << thread_id << "] Found " << links.size() 
                      << " links on page" << std::endl;
            
            // Extract unique domains from links
            std::unordered_set<std::string> unique_domains;
            for (const auto& link : links) {
                std::string link_domain = downloader.get_domain(link);
                if (!link_domain.empty()) {
                    unique_domains.insert(link_domain);
                }
            }
            std::cout << "[T" << thread_id << "] Extracted " << unique_domains.size() 
                      << " unique domains" << std::endl;
            
            // Store in thread-local buffer
            storage_manager.add_page(thread_id, domain, links);
            
            // Enqueue new links
            int new_urls = frontier.batch_enqueue(links);
            if (new_urls > 0) {
                std::cout << "[T" << thread_id << "] Enqueued " << new_urls 
                          << " new URLs" << std::endl;
            }
            
            pages_crawled.fetch_add(1);
            
        } else {
            // Queue is empty - busy wait with backoff
            if (frontier.queue_size() == 0) {
                if (backoff_ms < 500) {
                    backoff_ms *= 2;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
            }
        }
    }
    
    std::cout << "[T" << thread_id << "] Thread finished" << std::endl;
}

void ThreadManager::wait_completion() {
    for (auto& thread : workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }

#if DEBUG_DUMP
    // Dump frontier and visited set (safe - worker threads have joined)
    DumpUtils::dump_frontier(frontier.copy_queue());
    DumpUtils::dump_visited(frontier.copy_visited());
#endif
    
    frontier.mark_done();
    std::cout << "\n[CRAWL COMPLETE]" << std::endl;
    std::cout << "Total pages crawled: " << pages_crawled.load() << std::endl;
} 

void ThreadManager::dump_frontier_and_visited() {
#if DEBUG_DUMP
    auto q = frontier.get_queue_snapshot();
    auto v = frontier.get_visited_snapshot();
    DumpUtils::dump_frontier(q);
    DumpUtils::dump_visited(v);
#endif
}

int ThreadManager::get_pages_crawled() const {
    return pages_crawled.load();
}
