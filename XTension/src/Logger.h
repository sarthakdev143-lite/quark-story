#pragma once
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>

class Logger {
public:
    static void init(const std::string& filename = "StoryXT.log") {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) {
            m_file.open(filename, std::ios::out | std::ios::app);
        }
    }

    static void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) m_file.close();
    }

    static void info(const std::string& msg)   { log("INFO", msg); }
    static void error(const std::string& msg)  { log("ERROR", msg); }

private:
    static std::ofstream m_file;
    static std::mutex m_mutex;

    static void log(const std::string& level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) return;
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        m_file << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
               << " [" << level << "] " << msg << std::endl;
    }
};

inline std::ofstream Logger::m_file;
inline std::mutex Logger::m_mutex;