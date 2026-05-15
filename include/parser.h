#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

class Parser {
public:
    /**
     * Extract all links from HTML content
     * @param html HTML content to parse
     * @param base_url Base URL for resolving relative URLs
     * @return Vector of absolute URLs found
     */
    std::vector<std::string> extract_links(const std::string& html, 
                                           const std::string& base_url);
    
    /**
     * Extract domain name from full URL
     * @param url Full URL
     * @return Domain name only (e.g., "example.com")
     */
    std::string extract_domain(const std::string& url);
    
    /**
     * Check if URL is valid and should be crawled
     * @param url URL to validate
     * @return true if valid
     */
    bool is_valid_url(const std::string& url);
    
    /**
     * Normalize URL (remove fragments, standardize format)
     * @param url URL to normalize
     * @return Normalized URL
     */
    std::string normalize_url(const std::string& url);
    
    /**
     * Resolve relative URL against base URL
     * @param base Base URL
     * @param relative Relative URL
     * @return Absolute URL
     */
    std::string resolve_relative_url(const std::string& base, 
                                     const std::string& relative);

private:
    /**
     * Remove query parameters and fragments
     */
    std::string remove_query_fragment(const std::string& url);
    
    /**
     * Check if URL is relative
     */
    bool is_relative_url(const std::string& url);
};

#endif // PARSER_H
