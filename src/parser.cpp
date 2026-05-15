#include "parser.h"
#include "utils.h"
#include <regex>
#include <iostream>
#include <algorithm>

std::vector<std::string> Parser::extract_links(const std::string& html, 
                                               const std::string& base_url) {
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
            url = normalize_url(url);
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
    return !domain.empty();
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

std::string Parser::resolve_relative_url(const std::string& base, 
                                         const std::string& relative) {
    try {
        if (Utils::starts_with(relative, "http://") || 
            Utils::starts_with(relative, "https://")) {
            return relative;
        }
        
        // Extract base domain and path
        std::regex base_regex(R"(^(https?://[^/]+)(/?[^?#]*)?)");
        std::smatch match;
        
        if (!std::regex_search(base, match, base_regex)) {
            return base + "/" + relative;
        }
        
        std::string base_domain = match[1];
        std::string base_path = match[2];
        
        if (Utils::starts_with(relative, "/")) {
            // Absolute path
            return base_domain + relative;
        } else if (Utils::starts_with(relative, "./")) {
            // Current directory
            if (base_path.empty() || base_path.back() != '/') {
                base_path += "/";
            }
            return base_domain + base_path + relative.substr(2);
        } else if (Utils::starts_with(relative, "../")) {
            // Parent directory - simplified handling
            return base_domain + "/" + relative;
        } else {
            // Relative to current path
            if (base_path.empty() || base_path.back() != '/') {
                base_path += "/";
            }
            return base_domain + base_path + relative;
        }
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