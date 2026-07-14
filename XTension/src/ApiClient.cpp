/**
 * ApiClient.cpp - HTTP Client Implementation
 * 
 * Production-grade implementation with comprehensive error handling,
 * JSON parsing, and proper resource management.
 */

#include "ApiClient.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace StoryXT {

// Default timeout in seconds
constexpr long DEFAULT_TIMEOUT_SECONDS = 5L;

ApiClient::ApiClient(const std::string& baseUrl) 
    : m_baseUrl(baseUrl), m_curl(nullptr), m_timeout(DEFAULT_TIMEOUT_SECONDS) {
    
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Failed to initialize CURL: ") + curl_easy_strerror(res));
    }
    
    m_curl = curl_easy_init();
    if (!m_curl) {
        curl_global_cleanup();
        throw std::runtime_error("Failed to create CURL handle");
    }
}

ApiClient::~ApiClient() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
    curl_global_cleanup();
}

// Move constructor
ApiClient::ApiClient(ApiClient&& other) noexcept
    : m_baseUrl(std::move(other.m_baseUrl))
    , m_curl(other.m_curl)
    , m_timeout(other.m_timeout) {
    other.m_curl = nullptr;
}

// Move assignment
ApiClient& ApiClient::operator=(ApiClient&& other) noexcept {
    if (this != &other) {
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
        m_baseUrl = std::move(other.m_baseUrl);
        m_curl = other.m_curl;
        m_timeout = other.m_timeout;
        other.m_curl = nullptr;
    }
    return *this;
}

size_t ApiClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    if (!output) {
        return 0;
    }
    
    size_t totalSize = size * nmemb;
    try {
        output->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    } catch (...) {
        return 0; // Signal error to CURL
    }
}

std::string ApiClient::httpGet(const std::string& endpoint) {
    if (!m_curl) {
        throw std::runtime_error("CURL handle is not initialized");
    }
    
    std::string response;
    std::string url = m_baseUrl + endpoint;
    
    // Reset CURL handle for reuse
    curl_easy_reset(m_curl);
    
    // Set URL
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    
    // Set write callback
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    
    // Set timeout
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    
    // Follow redirects
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 3L);
    
    // Set user agent
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "StoryXT/1.0");
    
    // Perform request
    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("HTTP GET failed: ") + curl_easy_strerror(res));
    }
    
    // Check HTTP response code
    long httpCode = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (httpCode != 200) {
        throw std::runtime_error(std::string("HTTP error code: ") + std::to_string(httpCode));
    }
    
    return response;
}

std::optional<std::vector<StorySummary>> ApiClient::parseStoriesResponse(const std::string& jsonStr) {
    try {
        auto json = nlohmann::json::parse(jsonStr);
        
        // Check success flag
        if (!json.value("success", false)) {
            return std::nullopt;
        }
        
        const auto& data = json["data"];
        if (!data.is_array()) {
            return std::nullopt;
        }
        
        std::vector<StorySummary> stories;
        stories.reserve(data.size());
        
        for (const auto& item : data) {
            StorySummary story;
            story.id = item.value("id", 0);
            story.title = item.value("title", "");
            story.author = item.value("author", "");
            story.genre = item.value("genre", "");
            story.summary = item.value("summary", "");
            story.publishDate = item.value("publish_date", "");
            story.readTime = item.value("read_time", "");
            
            // Only add valid stories
            if (story.isValid()) {
                stories.push_back(std::move(story));
            }
        }
        
        return stories;
        
    } catch (const nlohmann::json::exception& e) {
        // Log JSON parse error (in production, use proper logging)
        return std::nullopt;
    }
}

std::optional<StoryDetail> ApiClient::parseStoryDetailResponse(const std::string& jsonStr) {
    try {
        auto json = nlohmann::json::parse(jsonStr);
        
        // Check success flag
        if (!json.value("success", false)) {
            return std::nullopt;
        }
        
        const auto& data = json["data"];
        if (!data.is_object()) {
            return std::nullopt;
        }
        
        StoryDetail detail;
        detail.id = data.value("id", 0);
        detail.title = data.value("title", "");
        detail.author = data.value("author", "");
        detail.genre = data.value("genre", "");
        detail.summary = data.value("summary", "");
        detail.publishDate = data.value("publish_date", "");
        detail.readTime = data.value("read_time", "");
        detail.content = data.value("content", "");
        
        // Validate the story
        if (detail.isValid()) {
            return detail;
        }
        
        return std::nullopt;
        
    } catch (const nlohmann::json::exception& e) {
        return std::nullopt;
    }
}

std::optional<std::vector<StorySummary>> ApiClient::fetchStories() {
    try {
        std::string response = httpGet("/api/v1/stories");
        return parseStoriesResponse(response);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<StoryDetail> ApiClient::fetchStoryDetail(int id) {
    if (id <= 0) {
        return std::nullopt;
    }
    
    try {
        std::string endpoint = "/api/v1/stories/" + std::to_string(id);
        std::string response = httpGet(endpoint);
        return parseStoryDetailResponse(response);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace StoryXT