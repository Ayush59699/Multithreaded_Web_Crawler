#pragma once
#include <queue>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#ifndef DEBUG_DUMP
// Set to 0 to disable dump calls at compile time (default 1 to enable).
#define DEBUG_DUMP 1
#endif

namespace DumpUtils {

void dump_frontier(const std::queue<std::string>& frontier,
                   const std::string& out_dir = "outputs/crawl");

void dump_visited(const std::unordered_set<std::string>& visited,
                  const std::string& out_dir = "outputs/crawl");

void dump_thread_local_buffer(
    const std::vector<std::pair<std::string, std::string>>& buffer,
    int thread_id,
    const std::string& out_dir = "outputs/crawl");

void dump_edge_list(const std::unordered_map<std::string, std::vector<std::string>>& graph,
                    const std::string& out_dir = "outputs/graph");

void dump_domain_stats(const std::unordered_map<std::string, size_t>& visit_count,
                       const std::unordered_map<std::string, size_t>& outgoing_links,
                       const std::string& out_dir = "outputs/graph");
void dump_pagerank(const std::unordered_map<std::string, double>& pagerank,
                   const std::string& out_dir = "outputs/analysis");

} // namespace DumpUtils