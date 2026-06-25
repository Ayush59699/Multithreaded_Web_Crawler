# 🕷️ Deep Dive Version 2: Benchmarking & Metrics Architecture

This second version of the project deep dive explores the advanced **performance profiling, benchmarking, and optimization** layers added to the Multithreaded Web Crawler.

---

## 1. The Need for Metrics

In high-performance C++ applications, assuming your code is fast because it uses threads is a common pitfall. To truly understand performance, we introduced a robust **Metrics Collection Architecture**.

We needed to answer:
1. *Where is the bottleneck?* Is it network latency, CPU parsing, or lock contention?
2. *Is multithreading actually helping?* How much faster is 4 threads compared to 1?
3. *What is the memory footprint?* Are we leaking memory during the graph build?

---

## 2. Core Components of the Profiling System

### 2.1 The `MetricsCollector`
The `MetricsCollector` class acts as the central telemetry hub. It captures:
- **Wall Clock Time:** Total end-to-end execution time.
- **CPU Time (User/System):** Using `getrusage` (POSIX) or `GetProcessTimes` (Windows/psapi) to track exact CPU cycles spent in user code vs. kernel space.
- **Peak Memory (Working Set):** Tracking the highest RAM usage during execution.
- **Network Idle Percentage:** Calculating how much time threads spent waiting on `libcurl` network I/O versus actively parsing HTML.

### 2.2 The `InstrumentedMutex`
Standard `std::mutex` instances block silently. We replaced our shared mutexes (specifically in the URL Frontier) with an `InstrumentedMutex`. 
This wrapper intercepts `lock()` calls, measures the duration using `std::chrono::high_resolution_clock`, and reports the wait time to the `MetricsCollector`.
* **Result:** We can explicitly prove that our Thread-Local Storage architecture has **0.00ms** of storage mutex wait time, validating our lock-free design!

---

## 3. The Benchmarking Suite (`benchmark_runner`)

To scientifically test optimizations, we built an automated `benchmark_runner` that executes multiple scenarios (trials) against a consistent dataset.

### Scenario A: Multithreading vs Single-Threading
The runner spins up identical crawls using 1 thread vs 4 threads. It logs the exact throughput scaling (Pages/sec) and Queue Contention.

### Scenario B: Naive vs Optimized Duplicate Detection
- **Naive:** Storing visited URLs in a `std::vector` and doing an $O(N)$ linear scan for every new link. As the crawl grows, CPU time skyrockets.
- **Optimized:** Using `std::unordered_set` for $O(1)$ constant-time hashing lookups. The `MetricsCollector` proves the optimized version dramatically reduces User CPU Time and increases Duplicate Rejection Rate.

### Scenario C: Graph Storage
- **Naive:** Flat edge list where relationships are constantly appended.
- **Optimized:** Adjacency-list mapping domains to outgoing link vectors, allowing for extremely fast iterative PageRank calculations.

---

## 4. The Synthetic Web Server (`synthetic_server.py`)

Real-world internet crawling is subject to unpredictable network latency, DNS resolution times, and server rate limits. You cannot benchmark C++ data structures accurately over the public internet.

To solve this, we created `synthetic_server.py`:
- It generates a **deterministic web graph** in memory.
- You can specify exact parameters (e.g., `--pages 200 --links 10`).
- It serves pages over `localhost` instantaneously.
- **Result:** The crawler's network wait time drops to near-zero, allowing the benchmark to stress-test the C++ CPU parser and lock contention directly.

---

## 5. Summary of Achievements

By combining the **Synthetic Server**, **Instrumented Mutexes**, and the **MetricsCollector**, we transformed a standard crawler into a professional-grade systems engineering project. The output metrics (such as throughput, contention ms, and CPU/Memory usage) can now be reliably used to validate any future architecture changes.
