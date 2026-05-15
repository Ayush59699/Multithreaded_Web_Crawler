// DumpUtils.cpp
// Implementation of dump helpers for crawler data structures.
// - Creates directories with std::filesystem::create_directories
// - Uses std::ofstream to write stable

#include "DumpUtils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace DumpUtils {

static void ensure_directory(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p, ec);
    if (ec) {
        std::cerr << "DumpUtils: failed to create directory " << p << " - " << ec.message() << "\n";
    }
}

void dump_frontier(const std::queue<std::string>& frontier, const std::string& out_dir) {
    fs::path dir(out_dir);
    ensure_directory(dir);
    fs::path file = dir / "frontier_dump.txt";

    std::ofstream out(file);
    if (!out.is_open()) {
        std::cerr << "DumpUtils: failed to open " << file << " for writing\n";
        return;
    }

    out << "URL FRONTIER\n";
    // Copy to iterate without modifying the original queue
    std::queue<std::string> q = frontier;
    size_t idx = 0;
    while (!q.empty()) {
        out << idx << ": " << q.front() << '\n';
        q.pop();
        ++idx;
    }
}

void dump_visited(const std::unordered_set<std::string>& visited, const std::string& out_dir) {
    fs::path dir(out_dir);
    ensure_directory(dir);
    fs::path file = dir / "visited_urls.txt";

    std::ofstream out(file);
    if (!out.is_open()) {
        std::cerr << "DumpUtils: failed to open " << file << " for writing\n";
        return;
    }

    out << "VISITED URLS (total = " << visited.size() << ")\n";

    // Make deterministic ordering for evaluator-friendly output
    std::vector<std::string> urls;
    urls.reserve(visited.size());
    for (const auto& u : visited) urls.push_back(u);
    std::sort(urls.begin(), urls.end());
    for (const auto& u : urls) out << u << '\n';
}

void dump_thread_local_buffer(const std::vector<std::pair<std::string, std::string>>& buffer,
                              int thread_id,
                              const std::string& out_dir) {
    fs::path dir(out_dir);
    ensure_directory(dir);
    std::ostringstream name;
    name << "thread_" << thread_id << "_buffer.txt";
    fs::path file = dir / name.str();

    std::ofstream out(file);
    if (!out.is_open()) {
        std::cerr << "DumpUtils: failed to open " << file << " for writing\n";
        return;
    }

    // Header
    out << "SRC_DOMAIN -> DEST_DOMAIN\n";
    for (const auto& p : buffer) {
        out << p.first << " -> " << p.second << '\n';
    }
}

void dump_edge_list(const std::unordered_map<std::string, std::vector<std::string>>& graph,
                    const std::string& out_dir) {
    fs::path dir(out_dir);
    ensure_directory(dir);
    fs::path file = dir / "edge_list.csv";

    std::ofstream out(file);
    if (!out.is_open()) {
        std::cerr << "DumpUtils: failed to open " << file << " for writing\n";
        return;
    }

    out << "source,destination\n";

    // Deterministic ordering: sort sources and destinations
    std::vector<std::string> sources;
    sources.reserve(graph.size());
    for (const auto& kv : graph) sources.push_back(kv.first);
    std::sort(sources.begin(), sources.end());

    for (const auto& src : sources) {
        const auto& dests = graph.at(src);
        std::vector<std::string> dcopy = dests;
        std::sort(dcopy.begin(), dcopy.end());
        for (const auto& dst : dcopy) {
            out << src << ',' << dst << '\n';
        }
    }
}

void dump_domain_stats(const std::unordered_map<std::string, size_t>& visit_count,
                       const std::unordered_map<std::string, size_t>& outgoing_links,
                       const std::string& out_dir) {
    fs::path dir(out_dir);
    ensure_directory(dir);
    fs::path file = dir / "domain_stats.csv";

    std::ofstream out(file);
    if (!out.is_open()) {
        std::cerr << "DumpUtils: failed to open " << file << " for writing\n";
        return;
    }

    out << "domain,visit_count,outgoing_links\n";

    // Merge keys to produce complete listing
    std::unordered_set<std::string> keys;
    for (const auto& kv : visit_count) keys.insert(kv.first);
    for (const auto& kv : outgoing_links) keys.insert(kv.first);

    std::vector<std::string> sorted_keys;
    sorted_keys.reserve(keys.size());
    for (const auto& k : keys) sorted_keys.push_back(k);
    std::sort(sorted_keys.begin(), sorted_keys.end());

    for (const auto& domain : sorted_keys) {
        size_t visits = 0;
        size_t outlinks = 0;
        auto itv = visit_count.find(domain);
        if (itv != visit_count.end()) visits = itv->second;
        auto ito = outgoing_links.find(domain);
        if (ito != outgoing_links.end()) outlinks = ito->second;
        out << domain << ',' << visits << ',' << outlinks << '\n';
    }
}

void dump_pagerank(const std::unordered_map<std::string, double>& pagerank,
                   const std::string& out_dir) {
    fs::path dir(out_dir);
    ensure_directory(dir);
    fs::path file = dir / "pagerank.csv";

    std::ofstream out(file);
    if (!out.is_open()) {
        std::cerr << "DumpUtils: failed to open " << file << " for writing\n";
        return;
    }

    out << "domain,pagerank\n";

    std::vector<std::string> domains;
    domains.reserve(pagerank.size());
    for (const auto& kv : pagerank) domains.push_back(kv.first);
    std::sort(domains.begin(), domains.end());

    // Fixed width for reproducibility
    out << std::fixed << std::setprecision(5);
    for (const auto& d : domains) {
        double score = pagerank.at(d);
        out << d << ',' << score << '\n';
    }
}

} // namespace DumpUtils
