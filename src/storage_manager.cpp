#include "storage_manager.h"
#include "utils.h"
#include "DumpUtils.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <algorithm>

StorageManager::StorageManager() : shared_storage_mutex(nullptr, InstrumentedMutex::Category::STORAGE) {}

void StorageManager::init(int num_threads, CrawlerConfig* config, MetricsCollector* metrics)
{
    config_ = config;
    metrics_ = metrics;
    shared_storage_mutex.set_metrics(metrics);
    
    thread_buffers.clear();
    naive_thread_buffers.clear();
    link_graph.clear();
    visit_count.clear();
    naive_link_graph.clear();
    naive_visit_count.clear();
    pagerank.clear();

    if (config_ && !config_->use_optimized_graph_storage)
    {
        naive_thread_buffers.resize(num_threads);
    }
    else
    {
        thread_buffers.resize(num_threads);
    }
}



ThreadLocalBuffer &StorageManager::get_thread_buffer(int thread_id)
{
    return thread_buffers[thread_id];
}

void StorageManager::add_page(int thread_id, const std::string &domain,
                              const std::vector<std::string> &outgoing_links)
{
    int target_thread_id = thread_id;
    bool lock_needed = false;

    if (config_)
    {
        if (!config_->enable_thread_local_storage)
        {
            target_thread_id = 0;
            lock_needed = true;
        }
        else if (!config_->enable_lock_minimized)
        {
            lock_needed = true;
        }
    }

    std::unique_ptr<InstrumentedLockGuard> lock;
    if (lock_needed)
    {
        lock = std::make_unique<InstrumentedLockGuard>(shared_storage_mutex);
    }

    // Extract domains from outgoing links
    std::vector<std::string> outgoing_domains;
    for (const auto &link : outgoing_links)
    {
        // Extract domain from URL
        std::string domain_from_link = link;
        size_t proto_end = domain_from_link.find("://");
        if (proto_end != std::string::npos)
        {
            domain_from_link = domain_from_link.substr(proto_end + 3);
        }

        // Remove path
        size_t path_start = domain_from_link.find('/');
        if (path_start != std::string::npos)
        {
            domain_from_link = domain_from_link.substr(0, path_start);
        }

        // Remove www. prefix
        if (Utils::starts_with(domain_from_link, "www."))
        {
            domain_from_link = domain_from_link.substr(4);
        }

        domain_from_link = Utils::to_lowercase(domain_from_link);

        if (!domain_from_link.empty())
        {
            outgoing_domains.push_back(domain_from_link);
            if (metrics_)
            {
                metrics_->record_graph_edge_inserted();
            }
        }
    }

    if (config_ && !config_->use_optimized_graph_storage)
    {
        auto &buffer = naive_thread_buffers[target_thread_id];
        for (const auto &out_domain : outgoing_domains)
        {
            buffer.edge_list.push_back({domain, out_domain});
        }
        
        bool found = false;
        for (auto &item : buffer.visit_count_list)
        {
            if (item.first == domain)
            {
                item.second++;
                found = true;
                break;
            }
        }
        if (!found)
        {
            buffer.visit_count_list.push_back({domain, 1});
        }
        
        if (std::find(buffer.local_domains.begin(), buffer.local_domains.end(), domain) == buffer.local_domains.end())
        {
            buffer.local_domains.push_back(domain);
        }
    }
    else
    {
        auto &buffer = thread_buffers[target_thread_id];
        // Store in thread-local buffer
        buffer.local_graph[domain] = outgoing_domains;
        buffer.local_visit_count[domain]++;
        buffer.local_domains.insert(domain);
    }
}

void StorageManager::merge_all_buffers()
{
    if (metrics_)
    {
        metrics_->start_graph_timer();
    }

    if (config_ && !config_->use_optimized_graph_storage)
    {
        merge_all_buffers_naive();
    }
    else
    {
        std::cout << "\n[INFO] Merging thread-local buffers (Optimized adjacency-list)..." << std::endl;

        for (const auto &buffer : thread_buffers)
        {
            // Merge graph
            for (const auto &[domain, links] : buffer.local_graph)
            {
                link_graph[domain] = links;
            }

            // Merge visit counts
            for (const auto &[domain, count] : buffer.local_visit_count)
            {
                visit_count[domain] += count;
            }
        }

        std::cout << "[INFO] Merged " << link_graph.size() << " unique domains" << std::endl;
    }

    if (metrics_)
    {
        metrics_->stop_graph_timer();
    }

#if DEBUG_DUMP
    dump_debug_outputs();
#endif
}

void StorageManager::merge_all_buffers_naive()
{
    std::cout << "\n[INFO] Merging thread-local buffers (Naive O(N^2) edge-list deduplication)..." << std::endl;

    std::vector<std::pair<std::string, std::string>> all_edges;
    std::vector<std::pair<std::string, int>> all_visits;

    for (const auto &buffer : naive_thread_buffers)
    {
        all_edges.insert(all_edges.end(), buffer.edge_list.begin(), buffer.edge_list.end());
        all_visits.insert(all_visits.end(), buffer.visit_count_list.begin(), buffer.visit_count_list.end());
    }

    naive_link_graph.clear();
    for (const auto &edge : all_edges)
    {
        bool exists = false;
        for (const auto &me : naive_link_graph)
        {
            if (me == edge)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            naive_link_graph.push_back(edge);
        }
    }

    naive_visit_count.clear();
    for (const auto &item : all_visits)
    {
        bool found = false;
        for (auto &global_item : naive_visit_count)
        {
            if (global_item.first == item.first)
            {
                global_item.second += item.second;
                found = true;
                break;
            }
        }
        if (!found)
        {
            naive_visit_count.push_back(item);
        }
    }

    std::cout << "[INFO] Merged " << naive_visit_count.size() << " unique domains naively" << std::endl;
}

void StorageManager::compute_pagerank(int iterations)
{
    if (metrics_)
    {
        metrics_->start_pagerank_timer();
    }

    if (config_ && !config_->use_optimized_pagerank)
    {
        compute_pagerank_naive(iterations);
    }
    else
    {
        pagerank_iteration(iterations);
    }

    if (metrics_)
    {
        metrics_->stop_pagerank_timer();
    }
}

void StorageManager::compute_pagerank_naive(int iterations)
{
    std::cout << "\n[INFO] Computing PageRank naively (" << iterations << " iterations)..." << std::endl;

    std::vector<std::string> nodes;
    if (config_ && !config_->use_optimized_graph_storage)
    {
        for (const auto &edge : naive_link_graph)
        {
            if (std::find(nodes.begin(), nodes.end(), edge.first) == nodes.end())
            {
                nodes.push_back(edge.first);
            }
            if (std::find(nodes.begin(), nodes.end(), edge.second) == nodes.end())
            {
                nodes.push_back(edge.second);
            }
        }
    }
    else
    {
        for (const auto &kv : link_graph)
        {
            if (std::find(nodes.begin(), nodes.end(), kv.first) == nodes.end())
            {
                nodes.push_back(kv.first);
            }
            for (const auto &dst : kv.second)
            {
                if (std::find(nodes.begin(), nodes.end(), dst) == nodes.end())
                {
                    nodes.push_back(dst);
                }
            }
        }
    }

    size_t N = nodes.size();
    if (N == 0)
    {
        std::cout << "[WARNING] No nodes to rank (naive)" << std::endl;
        return;
    }

    if (metrics_)
    {
        metrics_->record_pagerank_nodes(static_cast<int>(N));
    }

    std::map<std::string, double> pr;
    for (const auto &n : nodes)
    {
        pr[n] = 1.0 / static_cast<double>(N);
    }

    const double damping = 0.85;
    const double teleport = (1.0 - damping) / static_cast<double>(N);

    for (int iter = 0; iter < iterations; ++iter)
    {
        std::map<std::string, double> new_pr;
        for (const auto &n : nodes)
        {
            new_pr[n] = teleport;
        }

        for (const auto &src : nodes)
        {
            // Recompute out degree of src from graph every iteration
            int out_deg = 0;
            std::vector<std::string> outgoing;

            if (config_ && !config_->use_optimized_graph_storage)
            {
                for (const auto &edge : naive_link_graph)
                {
                    if (edge.first == src)
                    {
                        out_deg++;
                        outgoing.push_back(edge.second);
                    }
                }
            }
            else
            {
                auto it = link_graph.find(src);
                if (it != link_graph.end())
                {
                    out_deg = static_cast<int>(it->second.size());
                    outgoing = it->second;
                }
            }

            if (out_deg > 0)
            {
                double contribution = damping * (pr[src] / out_deg);
                for (const auto &dst : outgoing)
                {
                    new_pr[dst] += contribution;
                }
            }
        }

        // NO dangling mass redistribution and NO normalization (as requested for naive PR)
        pr = new_pr;
    }

    pagerank.clear();
    for (const auto &[node, score] : pr)
    {
        pagerank[node] = score;
    }

    std::cout << "[INFO] Naive PageRank computation complete" << std::endl;
}

void StorageManager::pagerank_iteration(int iterations)
{
    std::cout << "\n[INFO] Computing PageRank (" << iterations << " iterations)..." << std::endl;

    // 1) Build the full node set (keys + all destinations)
    std::unordered_set<std::string> nodes;
    nodes.reserve(link_graph.size() * 2);
    for (const auto &kv : link_graph)
    {
        nodes.insert(kv.first);
        for (const auto &dst : kv.second)
        {
            nodes.insert(dst);
        }
    }

    size_t N = nodes.size();
    if (N == 0)
    {
        std::cout << "[WARNING] No nodes to rank" << std::endl;
        return;
    }

    if (metrics_)
    {
        metrics_->record_pagerank_nodes(static_cast<int>(N));
    }

    std::cout << "[INFO] Total nodes (including destination-only): " << N << std::endl;

    // 2) Initialize pagerank for every node
    pagerank.clear();
    pagerank.reserve(N);
    for (const auto &n : nodes)
    {
        pagerank[n] = 1.0 / static_cast<double>(N);
    }

    const double damping = 0.85;
    const double teleport = (1.0 - damping) / static_cast<double>(N);

    for (int iter = 0; iter < iterations; ++iter)
    {
        std::unordered_map<std::string, double> new_pr;
        new_pr.reserve(N * 2);

        // Initialize with teleport term
        for (const auto &n : nodes)
        {
            new_pr[n] = teleport;
        }

        // Compute dangling mass (sources with zero outgoing links)
        double dangling_mass = 0.0;
        for (const auto &n : nodes)
        {
            auto it = link_graph.find(n);
            if (it == link_graph.end() || it->second.empty())
            {
                dangling_mass += pagerank[n];
            }
        }

        // Distribute contributions by iterating sources and their outgoing edges (O(E))
        for (const auto &n : nodes)
        {
            auto it = link_graph.find(n);
            if (it == link_graph.end() || it->second.empty())
            {
                continue;
            }

            const auto &outgoing = it->second;
            double outdeg = static_cast<double>(outgoing.size());
            double contribution = damping * (pagerank[n] / outdeg);

            for (const auto &dst : outgoing)
            {
                // dst must exist in `nodes` (we built nodes from all outgoing)
                new_pr[dst] += contribution;
            }
        }

        // Distribute dangling mass uniformly
        double dangling_share = damping * (dangling_mass / static_cast<double>(N));
        for (auto &kv : new_pr)
        {
            kv.second += dangling_share;
        }

        // Normalize to force numerical conservation = 1.0
        double sum = 0.0;
        for (const auto &kv : new_pr)
        {
            sum += kv.second;
        }
        if (sum > 0.0)
        {
            double inv_sum = 1.0 / sum;
            for (auto &kv : new_pr)
            {
                kv.second *= inv_sum;
            }
        }

        // Commit
        pagerank.swap(new_pr);
    }

    std::cout << "[INFO] PageRank computation complete" << std::endl;
    std::cout << "[INFO] Sum of all PageRank scores: " << std::fixed << std::setprecision(6);
    double total = 0.0;
    for (const auto &kv : pagerank)
    {
        total += kv.second;
    }
    std::cout << total << std::endl;

#if DEBUG_DUMP
    DumpUtils::dump_pagerank(pagerank);
#endif
}

void StorageManager::export_to_csv(const std::string &crawled_file,
                                   const std::string &ranking_file)
{
    // Export crawled pages
    std::ofstream crawled_csv(crawled_file);
    crawled_csv << "domain,outgoing_links,visit_count\n";

    if (config_ && !config_->use_optimized_graph_storage)
    {
        for (const auto &item : naive_visit_count)
        {
            std::string domain = item.first;
            int count = item.second;
            int outgoing_count = 0;
            for (const auto &edge : naive_link_graph)
            {
                if (edge.first == domain)
                {
                    outgoing_count++;
                }
            }
            crawled_csv << domain << "," << outgoing_count << "," << count << "\n";
        }
    }
    else
    {
        for (const auto &[domain, links] : link_graph)
        {
            int count = visit_count[domain];
            crawled_csv << domain << "," << links.size() << "," << count << "\n";
        }
    }

    crawled_csv.close();
    std::cout << "[INFO] Exported crawled pages to: " << crawled_file << std::endl;

    // Export PageRank results
    std::ofstream ranking_csv(ranking_file);
    ranking_csv << "domain,pagerank_score\n";
    ranking_csv << std::fixed << std::setprecision(6);

    for (const auto &[domain, score] : pagerank)
    {
        ranking_csv << domain << "," << score << "\n";
    }

    ranking_csv.close();
    std::cout << "[INFO] Exported PageRank results to: " << ranking_file << std::endl;
}

std::vector<std::string> StorageManager::get_all_domains() const
{
    std::vector<std::string> domains;
    if (config_ && !config_->use_optimized_graph_storage)
    {
        for (const auto &edge : naive_link_graph)
        {
            if (std::find(domains.begin(), domains.end(), edge.first) == domains.end())
            {
                domains.push_back(edge.first);
            }
            if (std::find(domains.begin(), domains.end(), edge.second) == domains.end())
            {
                domains.push_back(edge.second);
            }
        }
    }
    else
    {
        for (const auto &[domain, _] : link_graph)
        {
            domains.push_back(domain);
        }
    }
    return domains;
}

double StorageManager::get_pagerank(const std::string &domain) const
{
    auto it = pagerank.find(domain);
    return (it != pagerank.end()) ? it->second : 0.0;
}

int StorageManager::get_visit_count(const std::string &domain) const
{
    if (config_ && !config_->use_optimized_graph_storage)
    {
        for (const auto &item : naive_visit_count)
        {
            if (item.first == domain)
            {
                return item.second;
            }
        }
        return 0;
    }
    auto it = visit_count.find(domain);
    return (it != visit_count.end()) ? it->second : 0;
}

void StorageManager::dump_debug_outputs()
{
#if DEBUG_DUMP
    // 1) Dump thread-local buffers
    for (size_t tid = 0; tid < thread_buffers.size(); ++tid)
    {
        const auto &buf = thread_buffers[tid];
        // Convert local_graph (map -> vector) into vector of pairs
        std::vector<std::pair<std::string, std::string>> edges;
        for (const auto &[src, dests] : buf.local_graph)
        {
            for (const auto &dst : dests)
            {
                edges.emplace_back(src, dst);
            }
        }
        DumpUtils::dump_thread_local_buffer(edges, static_cast<int>(tid));
    }

    // 2) Dump merged edge list
    DumpUtils::dump_edge_list(link_graph);

    // 3) Prepare domain stats: visit_count (size_t) and outgoing link counts
    std::unordered_map<std::string, size_t> visits_sz;
    std::unordered_map<std::string, size_t> outgoing_sz;

    for (const auto &[domain, cnt] : visit_count)
    {
        visits_sz[domain] = static_cast<size_t>(cnt);
    }

    for (const auto &[domain, dests] : link_graph)
    {
        outgoing_sz[domain] = dests.size();
    }

    DumpUtils::dump_domain_stats(visits_sz, outgoing_sz);

    // 4) Dump PageRank
    DumpUtils::dump_pagerank(pagerank);
#endif
}