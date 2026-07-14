#include "ApiClient.h"
#include <stdexcept>
#include <sstream>

ApiClient::ApiClient(const std::string& baseUrl) : m_baseUrl(baseUrl) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_curl = curl_easy_init();
    if (!m_curl)
        throw std::runtime_error("Failed to initialize CURL");
}

ApiClient::~ApiClient() {
    if (m_curl)
        curl_easy_cleanup(m_curl);
    curl_global_cleanup();
}

size_t ApiClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string ApiClient::httpGet(const std::string& endpoint) {
    std::string response;
    std::string url = m_baseUrl + endpoint;
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 5L);  // 5 seconds timeout

    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }
    return response;
}

std::optional<std::vector<StorySummary>> ApiClient::fetchStories() {
    try {
        auto json = nlohmann::json::parse(httpGet("/api/stories"));
        if (!json.value("success", false)) return std::nullopt;
        std::vector<StorySummary> stories;
        for (const auto& item : json["data"]) {
            stories.push_back({
                item.value("id", 0),
                item.value("title", ""),
                item.value("author", ""),
                item.value("genre", ""),
                item.value("summary", ""),
                item.value("publish_date", ""),
                item.value("read_time", "")
            });
        }
        return stories;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<StoryDetail> ApiClient::fetchStoryDetail(int id) {
    try {
        auto json = nlohmann::json::parse(httpGet("/api/stories/" + std::to_string(id)));
        if (!json.value("success", false)) return std::nullopt;
        const auto& d = json["data"];
        StoryDetail detail;
        detail.id = d.value("id", 0);
        detail.title = d.value("title", "");
        detail.author = d.value("author", "");
        detail.genre = d.value("genre", "");
        detail.summary = d.value("summary", "");
        detail.publishDate = d.value("publish_date", "");
        detail.readTime = d.value("read_time", "");
        detail.content = d.value("content", "");
        return detail;
    } catch (...) {
        return std::nullopt;
    }
}