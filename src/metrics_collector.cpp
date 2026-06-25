#include "metrics_collector.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

// Timers
void MetricsCollector::start_total_timer() { total_start_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::stop_total_timer() { total_end_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::start_crawl_timer() { crawl_start_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::stop_crawl_timer() { crawl_end_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::start_graph_timer() { graph_start_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::stop_graph_timer() { graph_end_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::start_pagerank_timer() { pagerank_start_ = std::chrono::high_resolution_clock::now(); }
void MetricsCollector::stop_pagerank_timer() { pagerank_end_ = std::chrono::high_resolution_clock::now(); }

// Counters
void MetricsCollector::record_page_crawled() { pages_crawled_++; }
void MetricsCollector::record_url_processed() { urls_processed_++; }
void MetricsCollector::record_duplicate_url_rejected() { duplicate_urls_rejected_++; }
void MetricsCollector::record_url_enqueued() { urls_enqueued_++; }
void MetricsCollector::record_graph_edge_inserted() { graph_edges_inserted_++; }
void MetricsCollector::record_download_failed() { requests_failed_++; }
void MetricsCollector::record_pagerank_nodes(int count) { pagerank_nodes_.store(count); }


// Timing accumulators
void MetricsCollector::record_network_wait_time(double ms) {
    total_network_wait_us_ += static_cast<int64_t>(ms * 1000.0);
}
void MetricsCollector::record_queue_contention_time(double ms) {
    total_queue_contention_us_ += static_cast<int64_t>(ms * 1000.0);
}
void MetricsCollector::record_storage_mutex_wait_time(double ms) {
    total_storage_mutex_wait_us_ += static_cast<int64_t>(ms * 1000.0);
}

// System metrics
void MetricsCollector::capture_process_cpu_times() {
#ifdef _WIN32
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    if (GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        ULARGE_INTEGER uKernel, uUser;
        uKernel.LowPart = ftKernel.dwLowDateTime;
        uKernel.HighPart = ftKernel.dwHighDateTime;
        uUser.LowPart = ftUser.dwLowDateTime;
        uUser.HighPart = ftUser.dwHighDateTime;
        system_cpu_time_ms_ = uKernel.QuadPart / 10000.0;
        user_cpu_time_ms_ = uUser.QuadPart / 10000.0;
    }
#else
    struct tms time_buf;
    times(&time_buf);
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    user_cpu_time_ms_ = (double)time_buf.tms_utime * 1000.0 / ticks_per_sec;
    system_cpu_time_ms_ = (double)time_buf.tms_stime * 1000.0 / ticks_per_sec;
#endif
}

void MetricsCollector::capture_peak_memory() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        peak_memory_mb_ = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // ru_maxrss is in kilobytes on Linux
        peak_memory_mb_ = usage.ru_maxrss / 1024.0;
    }
#endif
}

void MetricsCollector::reset() {
    pages_crawled_ = 0;
    urls_processed_ = 0;
    urls_enqueued_ = 0;
    duplicate_urls_rejected_ = 0;
    graph_edges_inserted_ = 0;
    requests_failed_ = 0;
    pagerank_nodes_ = 0;
    total_network_wait_us_ = 0;
    total_queue_contention_us_ = 0;
    total_storage_mutex_wait_us_ = 0;
    user_cpu_time_ms_ = 0.0;
    system_cpu_time_ms_ = 0.0;
    peak_memory_mb_ = 0.0;
}

MetricsCollector::Results MetricsCollector::compute_results(const std::string& scenario_name, int num_threads) const {
    Results r;
    r.scenario_name = scenario_name;
    r.num_threads = num_threads;

    auto duration_ms = [](auto start, auto end) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    };

    r.wall_clock_time_ms = duration_ms(total_start_, total_end_);
    r.crawl_time_ms = duration_ms(crawl_start_, crawl_end_);
    r.graph_construction_time_ms = duration_ms(graph_start_, graph_end_);
    r.pagerank_time_ms = duration_ms(pagerank_start_, pagerank_end_);

    r.user_cpu_time_ms = user_cpu_time_ms_;
    r.system_cpu_time_ms = system_cpu_time_ms_;

    r.pages_crawled = pages_crawled_.load();
    r.urls_processed = urls_processed_.load();
    r.urls_enqueued = urls_enqueued_.load();
    r.duplicate_urls_rejected = duplicate_urls_rejected_.load();
    r.graph_edges_inserted = graph_edges_inserted_.load();
    r.requests_failed = requests_failed_.load();
    r.pagerank_nodes = pagerank_nodes_.load();


    // Throughput
    if (r.crawl_time_ms > 0) {
        r.pages_per_sec = r.pages_crawled / (r.crawl_time_ms / 1000.0);
        r.urls_per_sec = r.urls_processed / (r.crawl_time_ms / 1000.0);
    }
    if (r.graph_construction_time_ms > 0) {
        r.graph_edges_per_sec = r.graph_edges_inserted / (r.graph_construction_time_ms / 1000.0);
    }

    // Duplicate detection efficiency
    int total_discovered = r.urls_enqueued + r.duplicate_urls_rejected;
    if (total_discovered > 0) {
        r.duplicate_elimination_pct = (static_cast<double>(r.duplicate_urls_rejected) / total_discovered) * 100.0;
    }

    // Network wait
    r.total_network_wait_ms = total_network_wait_us_.load() / 1000.0;
    double total_active_thread_time = r.crawl_time_ms * num_threads;
    if (total_active_thread_time > 0) {
        r.network_idle_pct = std::max(0.0, 100.0 - (r.total_network_wait_ms / total_active_thread_time) * 100.0);
    }

    // Lock contention
    r.total_queue_contention_ms = total_queue_contention_us_.load() / 1000.0;
    r.total_storage_mutex_wait_ms = total_storage_mutex_wait_us_.load() / 1000.0;

    // Memory
    r.peak_memory_mb = peak_memory_mb_;
    if (r.pages_crawled > 0) {
        r.memory_per_page_kb = (r.peak_memory_mb * 1024.0) / r.pages_crawled;
    }

    return r;
}

// Compute standard deviation, average, median
static AggregatedResults::Stat calculate_stats(std::vector<double> vals) {
    AggregatedResults::Stat s;
    if (vals.empty()) return s;

    // Average
    double sum = std::accumulate(vals.begin(), vals.end(), 0.0);
    s.average = sum / vals.size();

    // Median
    std::sort(vals.begin(), vals.end());
    if (vals.size() % 2 == 0) {
        s.median = (vals[vals.size() / 2 - 1] + vals[vals.size() / 2]) / 2.0;
    } else {
        s.median = vals[vals.size() / 2];
    }

    // Stddev
    if (vals.size() > 1) {
        double sq_sum = 0.0;
        for (double v : vals) {
            sq_sum += (v - s.average) * (v - s.average);
        }
        s.stddev = std::sqrt(sq_sum / (vals.size() - 1));
    } else {
        s.stddev = 0.0;
    }

    return s;
}

AggregatedResults AggregatedResults::from_trials(const std::vector<MetricsCollector::Results>& trials) {
    AggregatedResults agg;
    if (trials.empty()) return agg;

    agg.scenario_name = trials[0].scenario_name;
    agg.num_threads = trials[0].num_threads;
    agg.num_trials = static_cast<int>(trials.size());

    // Gather vectors
    std::vector<double> wall_clocks, crawls, pages_sec, urls_sec, user_cpus, sys_cpus,
                        mem_peaks, mem_pages, net_waits, q_contents, store_mtxs,
                        dup_pcts, graph_times, graph_edges_sec, pr_times, pr_nodes_sec, req_fails;

    for (const auto& r : trials) {
        wall_clocks.push_back(r.wall_clock_time_ms);
        crawls.push_back(r.crawl_time_ms);
        pages_sec.push_back(r.pages_per_sec);
        urls_sec.push_back(r.urls_per_sec);
        user_cpus.push_back(r.user_cpu_time_ms);
        sys_cpus.push_back(r.system_cpu_time_ms);
        mem_peaks.push_back(r.peak_memory_mb);
        mem_pages.push_back(r.memory_per_page_kb);
        net_waits.push_back(r.total_network_wait_ms);
        q_contents.push_back(r.total_queue_contention_ms);
        store_mtxs.push_back(r.total_storage_mutex_wait_ms);
        dup_pcts.push_back(r.duplicate_elimination_pct);
        graph_times.push_back(r.graph_construction_time_ms);
        graph_edges_sec.push_back(r.graph_edges_per_sec);
        pr_times.push_back(r.pagerank_time_ms);
        // pagerank_nodes_per_sec requires nodes count
        if (r.pagerank_time_ms > 0) {
            pr_nodes_sec.push_back(r.pagerank_nodes / (r.pagerank_time_ms / 1000.0));
        } else {
            pr_nodes_sec.push_back(0.0);
        }
        req_fails.push_back(r.requests_failed);
    }

    agg.wall_clock_time_ms = calculate_stats(wall_clocks);
    agg.crawl_time_ms = calculate_stats(crawls);
    agg.pages_per_sec = calculate_stats(pages_sec);
    agg.urls_per_sec = calculate_stats(urls_sec);
    agg.user_cpu_time_ms = calculate_stats(user_cpus);
    agg.system_cpu_time_ms = calculate_stats(sys_cpus);
    agg.peak_memory_mb = calculate_stats(mem_peaks);
    agg.memory_per_page_kb = calculate_stats(mem_pages);
    agg.total_network_wait_ms = calculate_stats(net_waits);
    agg.total_queue_contention_ms = calculate_stats(q_contents);
    agg.total_storage_mutex_wait_ms = calculate_stats(store_mtxs);
    agg.duplicate_elimination_pct = calculate_stats(dup_pcts);
    agg.graph_construction_time_ms = calculate_stats(graph_times);
    agg.graph_edges_per_sec = calculate_stats(graph_edges_sec);
    agg.pagerank_time_ms = calculate_stats(pr_times);
    agg.pagerank_nodes_per_sec = calculate_stats(pr_nodes_sec);
    agg.requests_failed = calculate_stats(req_fails);

    return agg;
}

void AggregatedResults::export_csv(const std::vector<AggregatedResults>& all, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open CSV output file " << filepath << std::endl;
        return;
    }

    // Write header
    out << "scenario_name,num_threads,num_trials,"
        << "wall_clock_time_avg,wall_clock_time_median,wall_clock_time_stddev,"
        << "crawl_time_avg,crawl_time_median,crawl_time_stddev,"
        << "pages_per_sec_avg,pages_per_sec_median,pages_per_sec_stddev,"
        << "urls_per_sec_avg,urls_per_sec_median,urls_per_sec_stddev,"
        << "user_cpu_time_avg,user_cpu_time_median,user_cpu_time_stddev,"
        << "system_cpu_time_avg,system_cpu_time_median,system_cpu_time_stddev,"
        << "speedup_factor_avg,speedup_factor_median,speedup_factor_stddev,"
        << "parallel_efficiency_avg,parallel_efficiency_median,parallel_efficiency_stddev,"
        << "peak_memory_mb_avg,peak_memory_mb_median,peak_memory_mb_stddev,"
        << "memory_per_page_kb_avg,memory_per_page_kb_median,memory_per_page_kb_stddev,"
        << "queue_contention_ms_avg,queue_contention_ms_median,queue_contention_ms_stddev,"
        << "storage_mutex_wait_ms_avg,storage_mutex_wait_ms_median,storage_mutex_wait_ms_stddev,"
        << "duplicate_elimination_pct_avg,duplicate_elimination_pct_median,duplicate_elimination_pct_stddev,"
        << "graph_construction_time_ms_avg,graph_construction_time_ms_median,graph_construction_time_ms_stddev,"
        << "graph_edges_per_sec_avg,graph_edges_per_sec_median,graph_edges_per_sec_stddev,"
        << "pagerank_time_ms_avg,pagerank_time_ms_median,pagerank_time_ms_stddev,"
        << "pagerank_nodes_per_sec_avg,pagerank_nodes_per_sec_median,pagerank_nodes_per_sec_stddev,"
        << "requests_failed_avg,requests_failed_median,requests_failed_stddev\n";

    out << std::fixed << std::setprecision(4);

    for (const auto& a : all) {
        out << a.scenario_name << ","
            << a.num_threads << ","
            << a.num_trials << ","
            << a.wall_clock_time_ms.average << "," << a.wall_clock_time_ms.median << "," << a.wall_clock_time_ms.stddev << ","
            << a.crawl_time_ms.average << "," << a.crawl_time_ms.median << "," << a.crawl_time_ms.stddev << ","
            << a.pages_per_sec.average << "," << a.pages_per_sec.median << "," << a.pages_per_sec.stddev << ","
            << a.urls_per_sec.average << "," << a.urls_per_sec.median << "," << a.urls_per_sec.stddev << ","
            << a.user_cpu_time_ms.average << "," << a.user_cpu_time_ms.median << "," << a.user_cpu_time_ms.stddev << ","
            << a.system_cpu_time_ms.average << "," << a.system_cpu_time_ms.median << "," << a.system_cpu_time_ms.stddev << ","
            << a.speedup_factor.average << "," << a.speedup_factor.median << "," << a.speedup_factor.stddev << ","
            << a.parallel_efficiency.average << "," << a.parallel_efficiency.median << "," << a.parallel_efficiency.stddev << ","
            << a.peak_memory_mb.average << "," << a.peak_memory_mb.median << "," << a.peak_memory_mb.stddev << ","
            << a.memory_per_page_kb.average << "," << a.memory_per_page_kb.median << "," << a.memory_per_page_kb.stddev << ","
            << a.total_queue_contention_ms.average << "," << a.total_queue_contention_ms.median << "," << a.total_queue_contention_ms.stddev << ","
            << a.total_storage_mutex_wait_ms.average << "," << a.total_storage_mutex_wait_ms.median << "," << a.total_storage_mutex_wait_ms.stddev << ","
            << a.duplicate_elimination_pct.average << "," << a.duplicate_elimination_pct.median << "," << a.duplicate_elimination_pct.stddev << ","
            << a.graph_construction_time_ms.average << "," << a.graph_construction_time_ms.median << "," << a.graph_construction_time_ms.stddev << ","
            << a.graph_edges_per_sec.average << "," << a.graph_edges_per_sec.median << "," << a.graph_edges_per_sec.stddev << ","
            << a.pagerank_time_ms.average << "," << a.pagerank_time_ms.median << "," << a.pagerank_time_ms.stddev << ","
            << a.pagerank_nodes_per_sec.average << "," << a.pagerank_nodes_per_sec.median << "," << a.pagerank_nodes_per_sec.stddev << ","
            << a.requests_failed.average << "," << a.requests_failed.median << "," << a.requests_failed.stddev << "\n";
    }
}
