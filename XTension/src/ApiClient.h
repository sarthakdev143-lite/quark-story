#pragma once
#include <string>
#include <vector>
#include <optional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

struct StorySummary {
    int id;
    std::string title;
    std::string author;
    std::string genre;
    std::string summary;
    std::string publishDate;
    std::string readTime;
};

struct StoryDetail : StorySummary {
    std::string content;
};

class ApiClient {
public:
    explicit ApiClient(const std::string& baseUrl);
    ~ApiClient();

    // Non-copyable
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    std::optional<std::vector<StorySummary>> fetchStories();
    std::optional<StoryDetail> fetchStoryDetail(int id);

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output);
    std::string httpGet(const std::string& endpoint);

    std::string m_baseUrl;
    CURL* m_curl;
};