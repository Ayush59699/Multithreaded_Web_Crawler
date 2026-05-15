#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <string>
#include <thread>
#include <vector>
#include "url_frontier.h"
#include "storage_manager.h"
#include "downloader.h"
#include "parser.h"

/**
 * Manages worker thread pool
 * No locking - threads pull work from URLFrontier independently
 */
class ThreadManager {
public:
    /**
     * Start crawling with worker threads
     * @param num_threads Number of worker threads
     * @param max_pages Maximum pages to crawl
     * @param seed_url Starting URL
     * @param storage_manager Storage manager instance
     */
    void start(int num_threads, int max_pages, 
               const std::string& seed_url,
               StorageManager& storage_manager);
    
    /**
     * Wait for all threads to complete
     */
    void wait_completion();
    
    /**
     * Dump frontier and visited snapshots for debugging (guarded by DEBUG_DUMP)
     */
    void dump_frontier_and_visited();

    /**
     * Get number of pages crawled so far
     */
    int get_pages_crawled() const;

private:
    std::vector<std::thread> workers;
    URLFrontier frontier;
    std::atomic<int> pages_crawled{0};
    std::atomic<int> max_pages_limit{0};
    
    /**
     * Worker thread main loop
     * @param thread_id ID of this thread
     * @param storage_manager Reference to storage
     */
    void worker_loop(int thread_id, StorageManager& storage_manager);
};

#endif // THREAD_MANAGER_H
