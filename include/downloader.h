#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <string>
#include <vector>

class Downloader {
public:
    /**
     * Download HTML content from URL using libcurl
     * @param url URL to download
     * @return HTML content as string, empty string on failure
     */
    std::string download(const std::string& url);
    
    /**
     * Extract domain from URL
     * @param url Full URL
     * @return Domain name (e.g., "example.com")
     */
    std::string get_domain(const std::string& url);
    
    /**
     * Check if URL is valid
     * @param url URL to validate
     * @return true if valid HTTP/HTTPS URL
     */
    bool is_valid_url(const std::string& url);
    
    /**
     * Extract protocol from URL
     * @param url URL to parse
     * @return "https" or "http"
     */
    std::string get_protocol(const std::string& url);

private:
    /**
     * libcurl write callback for capturing response
     */
    static size_t write_callback(void* contents, size_t size, 
                                  size_t nmemb, std::string* userp);
};

#endif // DOWNLOADER_H
