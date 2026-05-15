#include "downloader.h"
#include "utils.h"
#include <curl/curl.h>
#include <regex>
#include <iostream>

// libcurl write callback
size_t Downloader::write_callback(void* contents, size_t size, 
                                  size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string Downloader::download(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "";
    }

    std::string readBuffer;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, 
                     "Mozilla/5.0 (X11; Linux x86_64) WebCrawler/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return "";
    }
    
    // Check HTTP status code
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    // Only return content for successful responses
    if (http_code >= 200 && http_code < 300) {
        return readBuffer;
    }
    
    return "";
}

std::string Downloader::get_domain(const std::string& url) {
    // Extract domain from URL
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
    
    return "";
}

bool Downloader::is_valid_url(const std::string& url) {
    // Check if URL starts with http:// or https://
    return Utils::starts_with(url, "http://") || 
           Utils::starts_with(url, "https://");
}

std::string Downloader::get_protocol(const std::string& url) {
    if (Utils::starts_with(url, "https://")) {
        return "https";
    }
    if (Utils::starts_with(url, "http://")) {
        return "http";
    }
    return "";
}
