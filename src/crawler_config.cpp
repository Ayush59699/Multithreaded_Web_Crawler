#include "crawler_config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Helper to trim whitespace from both ends of a string
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\",");
    return str.substr(first, (last - first + 1));
}

CrawlerConfig CrawlerConfig::from_json(const std::string& filepath) {
    CrawlerConfig config;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file " << filepath << ". Using defaults." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon + 1));

        if (key.empty() || val.empty()) continue;

        if (key == "seed_url") config.seed_url = val;
        else if (key == "max_pages") config.max_pages = std::stoi(val);
        else if (key == "num_threads") config.num_threads = std::stoi(val);
        else if (key == "enable_multithreading") config.enable_multithreading = (val == "true" || val == "1");
        else if (key == "use_optimized_duplicate_detection") config.use_optimized_duplicate_detection = (val == "true" || val == "1");
        else if (key == "use_optimized_graph_storage") config.use_optimized_graph_storage = (val == "true" || val == "1");
        else if (key == "use_optimized_pagerank") config.use_optimized_pagerank = (val == "true" || val == "1");
        else if (key == "enable_thread_local_storage") config.enable_thread_local_storage = (val == "true" || val == "1");
        else if (key == "enable_url_normalization") config.enable_url_normalization = (val == "true" || val == "1");
        else if (key == "enable_lock_minimized") config.enable_lock_minimized = (val == "true" || val == "1");
        else if (key == "enable_concurrent_download") config.enable_concurrent_download = (val == "true" || val == "1");
        else if (key == "scenario_name") config.scenario_name = val;
        else if (key == "num_trials") config.num_trials = std::stoi(val);
    }

    return config;
}

CrawlerConfig CrawlerConfig::default_optimized() {
    CrawlerConfig config;
    config.enable_multithreading = true;
    config.use_optimized_duplicate_detection = true;
    config.use_optimized_graph_storage = true;
    config.use_optimized_pagerank = true;
    config.enable_thread_local_storage = true;
    config.enable_url_normalization = true;
    config.enable_lock_minimized = true;
    config.enable_concurrent_download = true;
    config.scenario_name = "fully_optimized";
    return config;
}

CrawlerConfig CrawlerConfig::default_naive() {
    CrawlerConfig config;
    config.enable_multithreading = false;
    config.use_optimized_duplicate_detection = false;
    config.use_optimized_graph_storage = false;
    config.use_optimized_pagerank = false;
    config.enable_thread_local_storage = false;
    config.enable_url_normalization = false;
    config.enable_lock_minimized = false;
    config.enable_concurrent_download = false;
    config.num_threads = 1;
    config.scenario_name = "fully_naive";
    return config;
}

void CrawlerConfig::print() const {
    std::cout << "=== Crawler Configuration: " << scenario_name << " ===" << std::endl;
    std::cout << "  Seed URL:                      " << seed_url << std::endl;
    std::cout << "  Max Pages:                     " << max_pages << std::endl;
    std::cout << "  Num Threads:                   " << num_threads << std::endl;
    std::cout << "  Enable Multithreading:         " << (enable_multithreading ? "Yes" : "No") << std::endl;
    std::cout << "  Optimized Duplicate Detection: " << (use_optimized_duplicate_detection ? "Yes" : "No") << std::endl;
    std::cout << "  Optimized Graph Storage:       " << (use_optimized_graph_storage ? "Yes" : "No") << std::endl;
    std::cout << "  Optimized PageRank:            " << (use_optimized_pagerank ? "Yes" : "No") << std::endl;
    std::cout << "  Thread Local Storage:          " << (enable_thread_local_storage ? "Yes" : "No") << std::endl;
    std::cout << "  URL Normalization:             " << (enable_url_normalization ? "Yes" : "No") << std::endl;
    std::cout << "  Lock Minimized Strategy:       " << (enable_lock_minimized ? "Yes" : "No") << std::endl;
    std::cout << "  Concurrent Downloads:          " << (enable_concurrent_download ? "Yes" : "No") << std::endl;
    std::cout << "  Num Trials (benchmark only):   " << num_trials << std::endl;
    std::cout << "===========================================" << std::endl;
}
