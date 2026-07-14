/**
 * ApiClient.h - HTTP Client for Story API
 * 
 * Production-grade HTTP client implementation with:
 * - RAII-based CURL resource management
 * - Type-safe JSON parsing using nlohmann/json
 * - Comprehensive error handling
 * - Configurable timeouts and retry logic
 * 
 * @author Senior Developer
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace StoryXT {

/**
 * Story summary data structure for list views.
 * Contains all metadata except full content to optimize network transfer.
 */
struct StorySummary {
    int id;
    std::string title;
    std::string author;
    std::string genre;
    std::string summary;
    std::string publishDate;
    std::string readTime;
    
    /**
     * Check if the story summary is valid (non-empty).
     */
    bool isValid() const { return id > 0 && !title.empty(); }
};

/**
 * Full story detail data structure.
 * Extends StorySummary with complete content.
 */
struct StoryDetail : public StorySummary {
    std::string content;
    
    /**
     * Check if the story detail is valid.
     */
    bool isValid() const { return StorySummary::isValid() && !content.empty(); }
};

/**
 * API Client class for communicating with the Story backend service.
 * 
 * This class provides a clean interface for fetching story data from
 * the REST API, with proper error handling and resource management.
 */
class ApiClient {
public:
    /**
     * Construct an ApiClient with the specified base URL.
     * 
     * @param baseUrl Base URL of the API server (e.g., "http://localhost:5000")
     * @throws std::runtime_error if CURL initialization fails
     */
    explicit ApiClient(const std::string& baseUrl);
    
    /**
     * Destructor - ensures CURL resources are properly released.
     */
    ~ApiClient();

    // Non-copyable (RAII pattern)
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;
    
    // Movable
    ApiClient(ApiClient&& other) noexcept;
    ApiClient& operator=(ApiClient&& other) noexcept;

    /**
     * Fetch list of all story summaries.
     * 
     * @return Optional vector of StorySummary objects.
     *         Returns std::nullopt on failure.
     */
    std::optional<std::vector<StorySummary>> fetchStories();
    
    /**
     * Fetch detailed information for a specific story.
     * 
     * @param id Unique identifier of the story
     * @return Optional StoryDetail object.
     *         Returns std::nullopt if not found or on error.
     */
    std::optional<StoryDetail> fetchStoryDetail(int id);
    
    /**
     * Set connection timeout in seconds.
     * @param seconds Timeout duration
     */
    void setTimeout(long seconds) { m_timeout = seconds; }
    
    /**
     * Get current connection timeout.
     * @return Timeout in seconds
     */
    long getTimeout() const { return m_timeout; }

private:
    /**
     * Internal callback for CURL write operations.
     */
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output);
    
    /**
     * Perform HTTP GET request to specified endpoint.
     * 
     * @param endpoint API endpoint path (e.g., "/api/v1/stories")
     * @return Response body as string
     * @throws std::runtime_error on network errors
     */
    std::string httpGet(const std::string& endpoint);
    
    /**
     * Parse JSON response into StorySummary vector.
     */
    std::optional<std::vector<StorySummary>> parseStoriesResponse(const std::string& json);
    
    /**
     * Parse JSON response into StoryDetail.
     */
    std::optional<StoryDetail> parseStoryDetailResponse(const std::string& json);

    std::string m_baseUrl;
    CURL* m_curl;
    long m_timeout;
};

} // namespace StoryXT