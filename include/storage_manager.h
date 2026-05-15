#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <numeric>

/**
 * Per-thread local buffer for graph data
 * No locking - each thread has its own buffer
 */
struct ThreadLocalBuffer {
    std::unordered_map<std::string, std::vector<std::string>> local_graph;
    std::unordered_map<std::string, int> local_visit_count;
    std::unordered_set<std::string> local_domains;
};

/**
 * Storage manager with thread-local buffers
 * Main thread merges all buffers after crawling completes
 */
class StorageManager {
public:
    /**
     * Initialize storage with thread count
     * @param num_threads Number of worker threads
     */
    void init(int num_threads);
    
    /**
     * Get thread-local buffer for current thread
     * @param thread_id Thread ID
     * @return Reference to thread's local buffer
     */
    ThreadLocalBuffer& get_thread_buffer(int thread_id);
    
    /**
     * Record a page visit in thread-local buffer
     * @param thread_id Thread ID
     * @param domain Domain of page
     * @param outgoing_links Links found on page
     */
    void add_page(int thread_id, const std::string& domain, 
                  const std::vector<std::string>& outgoing_links);
    
    /**
     * Merge all thread-local buffers into global graph
     * Call AFTER all threads complete
     */
    void merge_all_buffers();
    
    /**
     * Compute PageRank using iterative algorithm
     * @param iterations Number of iterations (default 30)
     */
    void compute_pagerank(int iterations = 30);
    
    /**
     * Export results to CSV files
     * @param crawled_file Output file for crawled pages
     * @param ranking_file Output file for PageRank results
     */
    void export_to_csv(const std::string& crawled_file, 
                       const std::string& ranking_file);

    /**
     * Dump all debug outputs (thread buffers, merged graph, domain stats, PageRank)
     * This function is a convenience for main() and is guarded at compile-time
     * by the DEBUG_DUMP macro. It performs read-only copies and does not add
     * synchronization beyond existing container usage.
     */
    void dump_debug_outputs();
    
    /**
     * Get all domains in graph
     */
    std::vector<std::string> get_all_domains() const;
    
    /**
     * Get PageRank for specific domain
     */
    double get_pagerank(const std::string& domain) const;
    
    /**
     * Get visit count for domain
     */
    int get_visit_count(const std::string& domain) const;

private:
    std::vector<ThreadLocalBuffer> thread_buffers;
    
    // Merged graph after all threads complete
    std::unordered_map<std::string, std::vector<std::string>> link_graph;
    std::unordered_map<std::string, int> visit_count;
    std::unordered_map<std::string, double> pagerank;
    
    /**
     * Internal PageRank calculation
     */
    void pagerank_iteration(int iterations);
};

#endif // STORAGE_MANAGER_H
