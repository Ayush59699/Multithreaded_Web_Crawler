import argparse
import random
from http.server import HTTPServer, BaseHTTPRequestHandler
from socketserver import ThreadingTCPServer

class ThreadingHTTPServer(ThreadingTCPServer):
    allow_reuse_address = True

# Pre-generate pages in memory
PAGES = {}
GRAPH_STATS = {"pages": 0, "edges": 0}

def generate_web_graph(num_pages, links_per_page):
    global PAGES, GRAPH_STATS
    random.seed(42)  # Global seed for reproducible overall structure
    
    # Generate link targets for each page
    for i in range(num_pages):
        page_id = i
        # Seed locally for this page to keep link generation deterministic per page
        random.seed(page_id)
        
        num_links = random.randint(max(1, links_per_page - 5), links_per_page + 5)
        # Avoid linking to self, and link only to existing pages
        targets = []
        for _ in range(num_links):
            t = random.randint(0, num_pages - 1)
            if t != page_id and t not in targets:
                targets.append(t)
                
        # Generate some content
        lorem = (
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
            "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
            "nisi ut aliquip ex ea commodo consequat. "
        ) * 5
        
        # Build HTML content
        html_links = "".join(f'<p><a href="/page/{t}">Link to Page {t}</a></p>\n' for t in targets)
        html_content = f"""<!DOCTYPE html>
<html>
<head>
    <title>Page {page_id}</title>
</head>
<body>
    <h1>This is Page {page_id}</h1>
    <p>{lorem}</p>
    <div class="links">
        {html_links}
    </div>
</body>
</html>"""
        PAGES[f"/page/{page_id}"] = html_content
        GRAPH_STATS["edges"] += len(targets)
        
    GRAPH_STATS["pages"] = num_pages
    
    # Generate index page
    index_links = "".join(f'<p><a href="/page/{t}">Page {t}</a></p>\n' for t in range(min(10, num_pages)))
    index_content = f"""<!DOCTYPE html>
<html>
<head>
    <title>Index Page</title>
</head>
<body>
    <h1>Welcome to Synthetic Web Crawler Test Server</h1>
    <p>This is the seed page.</p>
    {index_links}
</body>
</html>"""
    PAGES["/"] = index_content

class SyntheticHTTPRequestHandler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    
    def log_message(self, format, *args):
        # Silence default logger to keep benchmark output clean
        pass

    def do_GET(self):
        global PAGES
        
        # Parse query string or simplify path
        path = self.path
        if "?" in path:
            path = path.split("?")[0]
            
        if path in PAGES:
            content = PAGES[path].encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Length', str(len(content)))
            self.end_headers()
            self.wfile.write(content)
        else:
            self.send_response(404)
            self.send_header('Content-Length', '0')
            self.end_headers()

def main():
    parser = argparse.ArgumentParser(description="Deterministic Synthetic HTTP Server")
    parser.add_argument("--pages", type=int, default=200, help="Total number of pages to generate")
    parser.add_argument("--links", type=int, default=10, help="Average links per page")
    parser.add_argument("--port", type=int, default=8080, help="Server port")
    args = parser.parse_args()
    
    print(f"Generating web graph of {args.pages} pages...")
    generate_web_graph(args.pages, args.links)
    print(f"Graph stats: {GRAPH_STATS['pages']} pages, {GRAPH_STATS['edges']} edges (avg degree: {GRAPH_STATS['edges']/GRAPH_STATS['pages']:.2f})")
    
    server_address = ('', args.port)
    httpd = ThreadingHTTPServer(server_address, SyntheticHTTPRequestHandler)
    print(f"Starting server on port {args.port}...")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")

if __name__ == "__main__":
    main()
