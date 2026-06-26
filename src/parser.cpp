#include "parser.h"
#include "utils.h"
#include <regex>
#include <iostream>
#include <algorithm>
#include <sstream>

std::vector<std::string> Parser::extract_links(const std::string& html, 
                                               const std::string& base_url,
                                               bool normalize) {
    std::vector<std::string> links;
    
    if (html.empty() || html.length() > 100000000) {  // 100MB safety limit
        return links;
    }
    
    try {
        // Regex to match href attributes in anchor tags
        std::regex href_regex(R"(href\s*=\s*[\"']([^\"']+)[\"'])");
        std::smatch match;
        
        std::string::const_iterator searchStart(html.cbegin());
        
        while (std::regex_search(searchStart, html.cend(), match, href_regex)) {
            std::string url = match[1];
            
            // Validate extracted URL
            if (url.empty() || url.length() > 10000) {
                searchStart = match.suffix().first;
                continue;
            }
            
            // Resolve relative URLs
            if (is_relative_url(url)) {
                url = resolve_relative_url(base_url, url);
            }
            
            // Normalize and validate
            if (normalize) {
                url = normalize_url(url);
            }
            if (is_valid_url(url)) {
                links.push_back(url);
            }
            
            searchStart = match.suffix().first;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in extract_links: " << e.what() << std::endl;
    }
    
    return links;
}

std::string Parser::extract_domain(const std::string& url) {
    try {
        std::regex domain_regex(R"(^https?://([^/]+))");
        std::smatch match;
        
        if (std::regex_search(url, match, domain_regex)) {
            std::string domain = match[1];
            // Remove 'www.' prefix if present
            if (Utils::starts_with(domain, "www.")) {
                domain = domain.substr(4);
            }
            return Utils::to_lowercase(domain);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in extract_domain: " << e.what() << std::endl;
    }
    
    return "";
}

bool Parser::is_valid_url(const std::string& url) {
    // Must start with http or https
    if (!Utils::starts_with(url, "http://") && 
        !Utils::starts_with(url, "https://")) {
        return false;
    }
    
    // Must not be too long
    if (url.length() > 10000) {
        return false;
    }
    
    // Must have a domain
    std::string domain = extract_domain(url);
    if (domain.empty()) {
        return false;
    }
    
    // Extract path and query/fragment
    std::string path = "/";
    std::string query_and_fragment = "";
    size_t scheme_pos = url.find("://");
    if (scheme_pos != std::string::npos) {
        size_t slash_pos = url.find('/', scheme_pos + 3);
        if (slash_pos != std::string::npos) {
            std::string rest = url.substr(slash_pos);
            split_path_query_fragment(rest, path, query_and_fragment);
        }
    }
    
    std::string path_lower = Utils::to_lowercase(path);
    std::string qf_lower = Utils::to_lowercase(query_and_fragment);
    
    // List of ignored extensions
    const std::vector<std::string> ignored_extensions = {
        ".css", ".js", ".ico", ".png", ".jpg", ".jpeg", 
        ".gif", ".svg", ".webp", ".pdf", ".zip", ".xml"
    };
    
    for (const auto& ext : ignored_extensions) {
        if (Utils::ends_with(path_lower, ext)) {
            return false;
        }
    }
    
    // Check for "rss" segment or ".rss" extension in path
    std::stringstream ss(path_lower);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (segment == "rss" || Utils::ends_with(segment, ".rss")) {
            return false;
        }
    }
    
    // Check for RSS in query/fragment safely (e.g. "=rss", "/rss", or ".rss")
    if (qf_lower.find("=rss") != std::string::npos || 
        qf_lower.find("/rss") != std::string::npos || 
        qf_lower.find(".rss") != std::string::npos) {
        return false;
    }
    
    return true;
}

std::string Parser::normalize_url(const std::string& url) {
    std::string normalized = url;
    
    try {
        // Remove fragment (everything after #)
        size_t fragment_pos = normalized.find('#');
        if (fragment_pos != std::string::npos) {
            normalized = normalized.substr(0, fragment_pos);
        }
        
        // Trim whitespace
        normalized = Utils::trim(normalized);
        
        // Convert to lowercase
        normalized = Utils::to_lowercase(normalized);
        
        // Normalize path segments (remove .. and .)
        size_t scheme_pos = normalized.find("://");
        if (scheme_pos != std::string::npos) {
            std::string scheme = normalized.substr(0, scheme_pos + 3);
            std::string rest = normalized.substr(scheme_pos + 3);
            size_t slash_pos = rest.find('/');
            if (slash_pos != std::string::npos) {
                std::string auth = rest.substr(0, slash_pos);
                std::string path = rest.substr(slash_pos);
                std::string path_only, qf;
                split_path_query_fragment(path, path_only, qf);
                normalized = scheme + auth + normalize_path(path_only) + qf;
            }
        }
        
        // Remove trailing slash from domain only (but keep path structure)
        if (normalized.length() > 0 && normalized.back() == '/') {
            // Check if this is just domain/
            std::regex domain_only_regex(R"(^https?://[^/]+/$)");
            if (std::regex_match(normalized, domain_only_regex)) {
                normalized = normalized.substr(0, normalized.length() - 1);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in normalize_url: " << e.what() << std::endl;
    }
    
    return normalized;
}

std::string Parser::normalize_path(const std::string& path) {
    std::vector<std::string> segments;
    std::string segment;
    std::stringstream ss(path);
    
    while (std::getline(ss, segment, '/')) {
        if (segment == "." || segment.empty()) {
            continue;
        }
        if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else {
            segments.push_back(segment);
        }
    }
    
    std::string result;
    for (const auto& seg : segments) {
        result += "/" + seg;
    }
    
    if (result.empty()) {
        return "/";
    }
    
    // Check if original path ended in trailing slash or directory dot reference
    bool has_trailing_slash = false;
    if (path.length() > 0) {
        if (path.back() == '/') {
            has_trailing_slash = true;
        } else if (path.length() >= 2 && path.substr(path.length() - 2) == "/.") {
            has_trailing_slash = true;
        } else if (path.length() >= 3 && path.substr(path.length() - 3) == "/..") {
            has_trailing_slash = true;
        }
    }
    
    if (has_trailing_slash) {
        result += "/";
    }
    
    return result;
}

void Parser::split_path_query_fragment(const std::string& url_or_path, 
                                       std::string& path, 
                                       std::string& query_and_fragment) {
    size_t pos = url_or_path.find_first_of("?#");
    if (pos != std::string::npos) {
        path = url_or_path.substr(0, pos);
        query_and_fragment = url_or_path.substr(pos);
    } else {
        path = url_or_path;
        query_and_fragment = "";
    }
}

std::string Parser::resolve_relative_url(const std::string& base, 
                                         const std::string& relative) {
    try {
        if (Utils::starts_with(relative, "http://") || 
            Utils::starts_with(relative, "https://")) {
            size_t scheme_pos = relative.find("://");
            std::string rel_scheme = relative.substr(0, scheme_pos + 3);
            std::string rest = relative.substr(scheme_pos + 3);
            size_t slash_pos = rest.find('/');
            if (slash_pos == std::string::npos) {
                return relative;
            }
            std::string rel_auth = rest.substr(0, slash_pos);
            std::string rel_path = rest.substr(slash_pos);
            
            std::string path_only, qf;
            split_path_query_fragment(rel_path, path_only, qf);
            return rel_scheme + rel_auth + normalize_path(path_only) + qf;
        }
        
        if (Utils::starts_with(relative, "?")) {
            // Find scheme of base URL
            size_t scheme_pos = base.find("://");
            if (scheme_pos == std::string::npos) {
                return base + relative;
            }
            std::string scheme = base.substr(0, scheme_pos + 3);
            std::string rest = base.substr(scheme_pos + 3);
            std::string authority, base_path;
            size_t slash_pos = rest.find('/');
            if (slash_pos == std::string::npos) {
                authority = rest;
                base_path = "/";
            } else {
                authority = rest.substr(0, slash_pos);
                base_path = rest.substr(slash_pos);
            }
            std::string base_path_only, base_qf;
            split_path_query_fragment(base_path, base_path_only, base_qf);
            return scheme + authority + normalize_path(base_path_only) + relative;
        }
        
        if (Utils::starts_with(relative, "#")) {
            size_t base_frag = base.find('#');
            std::string base_no_frag = (base_frag != std::string::npos) ? base.substr(0, base_frag) : base;
            return base_no_frag + relative;
        }
        
        // Find scheme of base URL
        size_t scheme_pos = base.find("://");
        if (scheme_pos == std::string::npos) {
            // Fallback if base is not a valid absolute URL
            return base + "/" + relative;
        }
        
        std::string scheme = base.substr(0, scheme_pos + 3);
        std::string rest = base.substr(scheme_pos + 3);
        
        std::string authority;
        std::string base_path;
        
        size_t slash_pos = rest.find('/');
        if (slash_pos == std::string::npos) {
            authority = rest;
            base_path = "/";
        } else {
            authority = rest.substr(0, slash_pos);
            base_path = rest.substr(slash_pos);
        }
        
        std::string base_path_only, base_qf;
        split_path_query_fragment(base_path, base_path_only, base_qf);
        
        // Find base directory
        size_t last_slash = base_path_only.find_last_of('/');
        std::string base_dir = "/";
        if (last_slash != std::string::npos) {
            base_dir = base_path_only.substr(0, last_slash + 1);
        }
        
        std::string rel_path, rel_qf;
        split_path_query_fragment(relative, rel_path, rel_qf);
        
        std::string resolved_path;
        if (Utils::starts_with(rel_path, "//")) {
            // Protocol relative
            std::string proto_rest = rel_path.substr(2);
            size_t proto_slash = proto_rest.find('/');
            if (proto_slash == std::string::npos) {
                return scheme.substr(0, scheme.length() - 1) + rel_path + rel_qf;
            }
            std::string proto_auth = proto_rest.substr(0, proto_slash);
            std::string proto_path = proto_rest.substr(proto_slash);
            return scheme + proto_auth + normalize_path(proto_path) + rel_qf;
        } else if (Utils::starts_with(rel_path, "/")) {
            // Root relative
            resolved_path = rel_path;
        } else {
            // Directory relative
            resolved_path = base_dir + rel_path;
        }
        
        return scheme + authority + normalize_path(resolved_path) + rel_qf;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in resolve_relative_url: " << e.what() << std::endl;
        return base + "/" + relative;
    }
}

bool Parser::is_relative_url(const std::string& url) {
    return !Utils::starts_with(url, "http://") && 
           !Utils::starts_with(url, "https://");
}

std::string Parser::remove_query_fragment(const std::string& url) {
    std::string result = url;
    
    try {
        // Remove query string
        size_t query_pos = result.find('?');
        if (query_pos != std::string::npos) {
            result = result.substr(0, query_pos);
        }
        
        // Remove fragment
        size_t fragment_pos = result.find('#');
        if (fragment_pos != std::string::npos) {
            result = result.substr(0, fragment_pos);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in remove_query_fragment: " << e.what() << std::endl;
    }
    
    return result;
}