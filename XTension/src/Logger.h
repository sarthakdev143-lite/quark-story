/**
 * Logger.h - Thread-Safe Logging Utility
 * 
 * Production-grade logging implementation with:
 * - Thread-safe file operations using mutex
 * - Timestamp formatting
 * - Multiple log levels (INFO, ERROR, WARNING, DEBUG)
 * - Automatic file management
 * 
 * @author Senior Developer
 * @version 1.0.0
 */

#pragma once

#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace StoryXT {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    /**
     * Initialize the logger with specified filename and log level.
     * @param filename Path to the log file
     * @param level Minimum log level to record
     */
    static void init(const std::string& filename = "StoryXT.log", 
                     LogLevel level = LogLevel::INFO) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) {
            m_file.open(filename, std::ios::out | std::ios::app);
            if (!m_file.is_open()) {
                std::cerr << "Failed to open log file: " << filename << std::endl;
            }
        }
        m_logLevel = level;
    }

    /**
     * Close the log file and release resources.
     */
    static void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.flush();
            m_file.close();
        }
    }

    /**
     * Log a debug message.
     * @param msg Message to log
     */
    static void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    
    /**
     * Log an info message.
     * @param msg Message to log
     */
    static void info(const std::string& msg) { log(LogLevel::INFO, msg); }
    
    /**
     * Log a warning message.
     * @param msg Message to log
     */
    static void warning(const std::string& msg) { log(LogLevel::WARNING, msg); }
    
    /**
     * Log an error message.
     * @param msg Message to log
     */
    static void error(const std::string& msg) { log(LogLevel::ERROR, msg); }

private:
    static std::ofstream m_file;
    static std::mutex m_mutex;
    static LogLevel m_logLevel;

    /**
     * Core logging function with level filtering.
     * @param level Log level of the message
     * @param msg Message to log
     */
    static void log(LogLevel level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Filter by log level
        if (level < m_logLevel) {
            return;
        }
        
        if (!m_file.is_open()) {
            return;
        }

        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        // Format log level string
        const char* levelStr = nullptr;
        switch (level) {
            case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
            case LogLevel::INFO:    levelStr = "INFO";  break;
            case LogLevel::WARNING: levelStr = "WARN";  break;
            case LogLevel::ERROR:   levelStr = "ERROR"; break;
        }

        // Write formatted log entry
        m_file << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
               << '.' << std::setfill('0') << std::setw(3) << ms.count()
               << " [" << levelStr << "] " << msg << std::endl;
        
        m_file.flush();
    }
};

// Static member definitions
inline std::ofstream Logger::m_file;
inline std::mutex Logger::m_mutex;
inline LogLevel Logger::m_logLevel = LogLevel::INFO;

} // namespace StoryXT