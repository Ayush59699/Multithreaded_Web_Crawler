# Multithreaded Web Crawler

A high-performance, lock-free web crawler written in C++17 that discovers and analyzes web pages using concurrent processing. The crawler efficiently extracts links, builds a domain graph, and computes PageRank scores for discovered domains.

## Features

- **Multithreaded Architecture** - Configurable worker thread pool for concurrent page crawling
- **Lock-Free Design** - Thread-local buffers minimize synchronization overhead and contention
- **Link Extraction** - Automatically extracts and resolves relative/absolute URLs from HTML
- **Domain Graph Analysis** - Builds inter-domain link graph from crawled content
- **PageRank Computation** - Calculates domain importance using iterative PageRank algorithm (30 iterations)
- **CSV Export** - Generates detailed reports of crawled pages and domain rankings
- **URL Validation** - Validates and normalizes URLs; tracks visited domains to avoid duplicates

## Prerequisites

- **CMake** 3.10 or higher
- **GCC/G++** with C++17 support
- **libcurl** development library (`libcurl4-openssl-dev`)
- **POSIX-compliant system** (Linux/Unix)

## Installation

### Quick Build

Run the automated build script:

```bash
./build.sh
```

This script will:

1. Check for required tools (cmake, gcc, libcurl)
2. Install missing dependencies (if needed)
3. Create build directory and compile
4. Verify the executable

### Manual Build

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Clean Build

Remove all build artifacts:

```bash
cd build
make clean-all
```

## Usage

### Basic Syntax

```bash
./crawler <seed_url> <max_pages> <num_threads>
```

### Arguments

| Argument      | Description                                     | Example               |
| ------------- | ----------------------------------------------- | --------------------- |
| `seed_url`    | Starting URL (must include http:// or https://) | `https://example.com` |
| `max_pages`   | Maximum number of pages to crawl                | `100`                 |
| `num_threads` | Number of worker threads (1-64)                 | `4`                   |

### Examples

Crawl example.com with 100 pages using 4 threads:

```bash
./crawler https://example.com 100 4
```

Crawl Wikipedia with 200 pages using 2 threads:

```bash
./crawler https://en.wikipedia.org 200 2
```

Crawl YouTube (large site) with 50 pages using 8 threads:

```bash
./crawler https://www.youtube.com 50 8
```

## Output

The crawler generates two CSV files in the current directory:

### `crawled_pages.csv`

Contains metadata for each page crawled:

```
url,domain,outgoing_link_count
https://example.com/page1,example.com,15
https://example.com/page2,example.com,8
https://other.com/page,other.com,5
```

### `pagerank_results.csv`

Contains PageRank scores for each discovered domain:

```
domain,pagerank_score
example.com,0.425630
other.com,0.185420
third.com,0.089150
```

Higher scores indicate more important domains based on link structure.

## Architecture

### Core Components

| Component          | Responsibility                                                               |
| ------------------ | ---------------------------------------------------------------------------- |
| **Downloader**     | Fetches HTML content from URLs using libcurl; parses and validates URLs      |
| **Parser**         | Extracts hyperlinks from HTML; normalizes and resolves relative URLs         |
| **URLFrontier**    | Thread-safe work queue managing URLs to crawl; prevents duplicate processing |
| **ThreadManager**  | Orchestrates worker thread pool and coordinates the crawling workflow        |
| **StorageManager** | Manages thread-local buffers; merges results and computes PageRank           |
| **Utils**          | String utilities (trim, split, case conversion, validation)                  |

### Design Philosophy

**Lock-Free Concurrency**: Each worker thread maintains its own buffer for the domain graph and visit counts. This eliminates lock contention and improves throughput. After all threads complete, the main thread merges all buffers into a global graph.

**Minimal Locking**: Only the URL frontier queue uses mutex protection. All other operations are completely lock-free.

**Atomic Counters**: Page tracking and progress monitoring uses atomic integers for thread-safe counters without locks.

## PageRank Algorithm

The crawler implements the standard PageRank algorithm with the following parameters:

- **Iterations**: 30 (configurable)
- **Damping Factor**: 0.85 (probability of following links vs. random teleportation)
- **Dangling Mass Distribution**: Nodes with no outgoing links redistribute their rank uniformly
- **Convergence**: Final scores normalized to sum to 1.0

### Formula

```
PR(A) = (1-d)/N + d * Σ(PR(T)/C(T))
```

Where:

- `d` = damping factor (0.85)
- `N` = total number of nodes
- `T` = pages linking to A
- `C(T)` = number of outgoing links from T

## Performance Characteristics

- **Throughput**: Scales with number of threads (reduces lock contention)
- **Memory**: ~O(pages × avg_links_per_page) for thread-local buffers
- **CPU**: Optimized with `-O2` compiler flag

### Typical Performance

Crawling 200 pages from a medium-sized website with 4 threads typically completes in 30-60 seconds depending on network conditions and server response times.

## Example Workflow

```bash
$ ./crawler https://example.com 50 4

╔═══════════════════════════════════════════════════════════╗
║    Multithreaded Web Crawler (Lock-Free)                ║
╚═══════════════════════════════════════════════════════════╝

[INFO] Starting crawler with 4 threads...
[INFO] Seed URL: https://example.com
[INFO] Max pages: 50
[INFO] Worker thread 1 started
[INFO] Worker thread 2 started
[INFO] Worker thread 3 started
[INFO] Worker thread 4 started

[INFO] Computing PageRank (30 iterations)...
[INFO] Total nodes (including destination-only): 127
[INFO] PageRank computation complete
[INFO] Sum of all PageRank scores: 1.000000

╔═══════════════════════════════════════════════════════════╗
║                    CRAWL FINISHED                         ║
╚═══════════════════════════════════════════════════════════╝

[RESULTS]
  Pages crawled:   50
  CSV files generated:
    - crawled_pages.csv
    - pagerank_results.csv
```

## Limitations & Future Work

### Current Limitations

- No robots.txt compliance checking
- No rate limiting or crawl delays
- No cookie/session handling
- Limited JavaScript execution (static content only)
- No distributed crawling

### Future Enhancements

- Robots.txt parser and compliance
- Configurable crawl delays and politeness settings
- Distributed crawling across multiple machines
- Advanced filtering (file types, domain restrictions)
- Detailed performance metrics and logging
- Error recovery and retry mechanisms

## Troubleshooting

### Build Fails

- Ensure CMake 3.10+ is installed: `cmake --version`
- Install libcurl development headers: `sudo apt-get install libcurl4-openssl-dev`
- Check GCC version supports C++17: `g++ --version`

### Crawler Hangs

- Verify seed URL is valid and accessible
- Check network connectivity
- Try increasing thread count for timeout issues

### Invalid CSV Output

- Verify the seed URL is valid and returns HTML
- Check that max_pages is greater than 0
- Ensure sufficient disk space for output files

## Building from Source

### Detailed Build Steps

```bash
# Clone repository
git clone https://github.com/kartik-sc/multithreaded-web-crawler.git
cd multithreaded-web-crawler

# Build
./build.sh

# Navigate to build directory
cd build

# Run crawler
./crawler https://example.com 100 4
```

## License

[Specify your license here, e.g., MIT, Apache 2.0, etc.]

## Author

[Your Name/Organization]

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.
