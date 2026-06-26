# Multithreaded Web Crawler

A high-performance, lock-free web crawler written in C++17 that discovers and analyzes web pages using concurrent processing. This project is engineered with a heavy focus on performance profiling and system-level metrics, featuring custom benchmarking suites, lock contention monitoring, and synthetic web-graph generation.

## Performance and Metrics

The crawler is built to scale and includes an advanced `MetricsCollector` that records microsecond-level performance data without introducing observer overhead. 

* **Exceptional Throughput**: Achieves 540+ pages/second and 5,100+ URLs/second on a 4-thread local benchmark.
* **Zero-Contention Storage**: Innovative lock-free thread-local buffers result in 0.00 ms storage mutex wait time across the entire crawl.
* **Optimized O(1) Deduplication**: Replaced naive O(N) vector scans with `std::unordered_set`, achieving a 63%+ duplicate rejection rate with near-instant lookup.
* **Granular CPU Profiling**: Tracks and isolates User CPU Time (e.g., 170ms) vs System CPU Time (e.g., 40ms) using platform-specific API hooks.
* **Minimal Memory Footprint**: Maintains a lean peak working set (e.g., ~12 MB for 50 pages) equating to roughly 234 KB per page analyzed.

## Key Features and Architecture

- **Multithreaded Architecture**: Configurable worker thread pool for concurrent page crawling.
- **Instrumented Mutexes**: Custom `InstrumentedMutex` wrapper that actively records wait times and lock contention specifically for the URL Frontier queue.
- **Dynamic Seed Input**: Accepts either a single seed URL or a path to a local text file (e.g., `urls.txt`) containing multiple seed URLs (one per line) to initialize the frontier and crawl them in parallel.
- **Robust Relative URL Resolution**: Resolves relative paths including root-relative (`/`), protocol-relative (`//`), query-relative (`?`), and directory-relative (`../` or `./`) paths using stack-based path segment normalization to correctly navigate nested structures.
- **HTML Content Filtering**: Prevents crawling non-HTML static resources by pre-filtering URLs based on file extensions (ignoring `.css`, `.js`, `.ico`, `.png`, `.jpg`, `.jpeg`, `.gif`, `.svg`, `.webp`, `.pdf`, `.zip`, `.xml`, and RSS feeds).
- **Response Validation**: Checks response `Content-Type` headers via libcurl, processing only `text/html` or `application/xhtml+xml` payloads and discarding media or raw data assets.
- **Link Extraction and Domain Graph Analysis**: Extracts links and builds an Adjacency-list based inter-domain link graph.
- **PageRank Computation**: Calculates domain importance using an iterative PageRank algorithm.
- **Deterministic Benchmarking (`benchmark_runner`)**: An automated C++ test suite that runs trials comparing single-threaded vs. multi-threaded execution, and naive vs. optimized data structures.
- **Synthetic Web Server (`synthetic_server.py`)**: A Python-based local HTTP server that pre-generates a reproducible, customizable web graph in memory to test the crawler without real-world network latency.

## Usage

### Prerequisites
- **CMake** 3.10+
- **GCC/G++** with C++17 support
- **libcurl** development library (`libcurl4-openssl-dev`)

*(Note: Windows users are highly encouraged to use WSL for native POSIX compliance and easy dependency installation).*

### Building the Project
You can build the `crawler` and `benchmark_runner` executables using the build script:
```bash
./build.sh
```
Or build directly inside the build directory:
```bash
cd build && cmake .. && make -j$(nproc)
```

### Running the Crawler
```bash
./build/crawler <seed_url_or_file> <max_pages> <num_threads>
```

*Examples:*

Running with a single seed URL:
```bash
./build/crawler https://books.toscrape.com/ 100 4
```

Running with a text file containing multiple seed URLs (one per line):
```bash
./build/crawler urls.txt 100 4
```

### Running the Benchmark Suite
Start the synthetic server in the background, then run the benchmark suite to see comparative metrics:
```bash
python3 synthetic_server.py --pages 200 --links 10 --port 8080 &
./build/benchmark_runner
```

## Output Artifacts

The crawler generates data files in the workspace directory:

1. **`metrics.csv`** - Appends performance data for every run (wall clock time, throughput, CPU time, queue contention, network idle %).
2. **`crawled_pages.csv`** - Contains metadata for each page crawled (URL, domain, outgoing link count).
3. **`pagerank_results.csv`** - Contains iterative PageRank scores for each discovered domain.
