#!/usr/bin/env python3
"""
Web Crawler Metrics Analyzer (Pure Python)
Generates text-based metrics and simple HTML visualization
"""

import csv
from pathlib import Path
from statistics import mean, stdev

# Read metrics
csv_file = Path('build/metrics.csv')
if not csv_file.exists():
    print(f"Error: {csv_file} not found")
    exit(1)

data = []
with open(csv_file, 'r') as f:
    reader = csv.DictReader(f)
    for row in reader:
        row['num_threads'] = int(row['num_threads'])
        row['total_ms'] = int(row['total_ms'])
        row['pages_crawled'] = int(row['pages_crawled'])
        row['throughput'] = float(row['throughput'])
        data.append(row)

print("=" * 70)
print("CRAWLER METRICS SUMMARY".center(70))
print("=" * 70)
print()

# Print table
print(f"{'URL':<45} {'Threads':<8} {'Pages':<8} {'Time(s)':<10} {'Throughput':<10}")
print("-" * 70)

for row in data:
    domain = row['seed_url'].split('/')[2] if '://' in row['seed_url'] else row['seed_url']
    domain = domain[:40] + "..." if len(domain) > 40 else domain
    time_sec = row['total_ms'] / 1000
    print(f"{domain:<45} {row['num_threads']:<8} {row['pages_crawled']:<8} {time_sec:<10.2f} {row['throughput']:<10.2f}")

print()
print("=" * 70)
print("STATISTICS".center(70))
print("=" * 70)

print(f"Total runs:              {len(data)}")
print(f"Total pages crawled:     {sum(r['pages_crawled'] for r in data)}")
print(f"Average throughput:      {mean(r['throughput'] for r in data):.2f} pages/sec")
print(f"Max throughput:          {max(r['throughput'] for r in data):.2f} pages/sec")
print(f"Min throughput:          {min(r['throughput'] for r in data):.2f} pages/sec")
if len(data) > 1:
    print(f"Throughput stdev:        {stdev(r['throughput'] for r in data):.2f} pages/sec")
print(f"Average crawl time:      {mean(r['total_ms'] for r in data) / 1000:.2f} seconds")

print()

# Generate HTML report
html_content = """<!DOCTYPE html>
<html>
<head>
    <title>Crawler Metrics Report</title>
    <style>
        * { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
        body { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; }
        .container { max-width: 1200px; margin: auto; background: white; border-radius: 10px; padding: 30px; box-shadow: 0 10px 30px rgba(0,0,0,0.3); }
        h1 { color: #333; text-align: center; border-bottom: 3px solid #667eea; padding-bottom: 15px; }
        h2 { color: #667eea; margin-top: 30px; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th { background: #667eea; color: white; padding: 12px; text-align: left; font-weight: bold; }
        td { padding: 10px 12px; border-bottom: 1px solid #ddd; }
        tr:hover { background: #f5f5f5; }
        .stat-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-box { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 8px; text-align: center; }
        .stat-value { font-size: 28px; font-weight: bold; }
        .stat-label { font-size: 12px; opacity: 0.9; margin-top: 5px; }
        .chart-row { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        .chart { background: #f9f9f9; padding: 20px; border-radius: 8px; border-left: 4px solid #667eea; }
        .bar-chart-item { display: flex; align-items: center; margin: 10px 0; }
        .bar-label { width: 150px; font-weight: 500; }
        .bar-container { flex: 1; background: #e0e0e0; height: 25px; border-radius: 4px; position: relative; overflow: hidden; }
        .bar-fill { background: linear-gradient(90deg, #667eea, #764ba2); height: 100%; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold; font-size: 12px; }
        .footer { text-align: center; margin-top: 40px; color: #999; font-size: 12px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 Web Crawler Performance Metrics</h1>
        
        <h2>📊 Statistics</h2>
        <div class="stat-grid">
"""

# Add statistics
stats = {
    'Total Runs': len(data),
    'Total Pages': sum(r['pages_crawled'] for r in data),
    'Avg Throughput': f"{mean(r['throughput'] for r in data):.2f} p/s",
    'Max Throughput': f"{max(r['throughput'] for r in data):.2f} p/s",
}

for label, value in stats.items():
    html_content += f"""        <div class="stat-box">
            <div class="stat-value">{value}</div>
            <div class="stat-label">{label}</div>
        </div>
"""

html_content += """        </div>
        
        <h2>📈 Detailed Results</h2>
        <table>
            <tr>
                <th>Seed URL</th>
                <th>Threads</th>
                <th>Pages Crawled</th>
                <th>Total Time (s)</th>
                <th>Throughput (p/s)</th>
                <th>Efficiency</th>
            </tr>
"""

for row in data:
    time_sec = row['total_ms'] / 1000
    efficiency = row['throughput'] / row['num_threads']
    domain = row['seed_url'].split('/')[2] if '://' in row['seed_url'] else row['seed_url']
    html_content += f"""            <tr>
                <td><code>{domain}</code></td>
                <td>{row['num_threads']}</td>
                <td>{row['pages_crawled']}</td>
                <td>{time_sec:.2f}</td>
                <td>{row['throughput']:.2f}</td>
                <td>{efficiency:.3f}</td>
            </tr>
"""

html_content += """        </table>
        
        <h2>📊 Visual Analysis</h2>
        <div class="chart-row">
"""

# Throughput chart
html_content += """            <div class="chart">
                <h3>Throughput per Run</h3>
"""

max_throughput = max(r['throughput'] for r in data)
for row in data:
    domain = row['seed_url'].split('/')[2] if '://' in row['seed_url'] else row['seed_url']
    percentage = (row['throughput'] / max_throughput) * 100
    html_content += f"""                <div class="bar-chart-item">
                    <div class="bar-label">{domain[:20]}... ({row['num_threads']}T)</div>
                    <div class="bar-container">
                        <div class="bar-fill" style="width: {percentage}%;">{row['throughput']:.2f}</div>
                    </div>
                </div>
"""

html_content += """            </div>
            
            <div class="chart">
                <h3>Total Crawl Time</h3>
"""

max_time = max(r['total_ms'] for r in data)
for row in data:
    domain = row['seed_url'].split('/')[2] if '://' in row['seed_url'] else row['seed_url']
    percentage = (row['total_ms'] / max_time) * 100
    time_sec = row['total_ms'] / 1000
    html_content += f"""                <div class="bar-chart-item">
                    <div class="bar-label">{domain[:20]}... ({row['num_threads']}T)</div>
                    <div class="bar-container">
                        <div class="bar-fill" style="width: {percentage}%;">{time_sec:.1f}s</div>
                    </div>
                </div>
"""

html_content += """            </div>
        </div>
        
        <div class="footer">
            <p>Report generated from metrics.csv | Web Crawler Performance Analysis</p>
        </div>
    </div>
</body>
</html>
"""

# Save HTML
html_file = Path('build/metrics_report.html')
with open(html_file, 'w') as f:
    f.write(html_content)

print(f"✓ HTML report saved to: {html_file}")
print()
print("=" * 70)
print("To view the report in your browser:")
print(f"  open build/metrics_report.html")
print("=" * 70)
