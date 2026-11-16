#pragma once

#include <chrono>
#include <cstdio>
//#include <exception>
//#include <filesystem>
//#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

#define CSVLOG_VERSION_MAJOR 0
#define CSVLOG_VERSION_MINOR 2
#define CSVLOG_VERSION_PATCH 0

#if defined(__cpp_lib_print)
#include <format>
#include <print>
// #define CSVLOG_PRINT			std::print
#define CSVLOG_PRINT_LINE std::println
#define CSVLOG_MAKE_STRING std::format
#define CSVLOG_FORMAT_STRING std::format_string
#else
#define FMT_HEADER_ONLY
#include <fmt/chrono.h>
#include <fmt/format.h>
// #define CSVLOG_PRINT			fmt::print
#define CSVLOG_PRINT_LINE fmt::println
#define CSVLOG_MAKE_STRING fmt::format
#define CSVLOG_FORMAT_STRING fmt::format_string
#endif

//namespace fs = std::filesystem;

#ifndef CSVLOG_BEGIN_NAMESPACE
#define CSVLOG_BEGIN_NAMESPACE \
    namespace csvlog {         \
    inline namespace v0 {

#endif

#ifndef CSVLOG_END_NAMESPACE
#define CSVLOG_END_NAMESPACE \
    }                        \
    }
#endif

CSVLOG_BEGIN_NAMESPACE

class Logger final {
  public:
    struct Config {
        Config() {
        }

        std::string filename_prefix{"app"};
        std::string filename_extension{".csv"};
        std::string header_row{};
        size_t max_row_count{60'000};  // Max # rows, excluding header
        size_t max_file_size{300 * 1024 * 1024};    // 300 MB
        std::chrono::hours roll_time_interval{24};  // 24 hours

        friend class Logger;

      private:
        std::string get_filename_with_timestamp() const {
            auto now = std::chrono::system_clock::now();
            auto tp = std::chrono::time_point_cast<std::chrono::seconds>(now);

            return CSVLOG_MAKE_STRING("{}_{:%Y%m%d_%H%M%S}{}", filename_prefix,
                                      tp, filename_extension);
        }
    };

  private:
    Config config;
    FILE* file = NULL;
    std::mutex write_mutex{};
    std::chrono::steady_clock::time_point next_roll_time{};
    std::string current_filename{};
    size_t current_row_count = 0;
    size_t current_file_size = 0;

    void close_file() {
        if (file) {
            fclose(file);
        }

        file = NULL;
    }

    void open_new_file() {
        close_file();

        current_filename = config.get_filename_with_timestamp();
        file = std::fopen(current_filename.c_str(), "w");

        if (file == NULL) {
            throw std::runtime_error(CSVLOG_MAKE_STRING(
                "Failed to open file: {}", current_filename));
        }

        current_row_count = 0;
        current_file_size = 0;
        next_roll_time =
            std::chrono::steady_clock::now() + config.roll_time_interval;

        if (!config.header_row.empty()) {
            CSVLOG_PRINT_LINE(file, "Date, Time, {}", config.header_row);
        }
    }

    bool should_roll_by_rows() const {
        return current_row_count >= config.max_row_count;
    }

    bool should_roll_by_size() const {
        return current_file_size >= config.max_file_size;
    }

    bool should_roll_by_time() const {
        return std::chrono::steady_clock::now() >= next_roll_time;
    }

    void check_and_roll() {
        if (!file || should_roll_by_rows() || should_roll_by_size() ||
            should_roll_by_time()) {
            open_new_file();
        }
    }

  public:
    explicit Logger(const Config& cfg = Config()) : config(cfg) {
        open_new_file();
    }

    ~Logger() {
        if (file) {
            fclose(file);
        }
    }

    // Delete copy operations
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Allow move operations
    Logger(Logger&&) noexcept = default;
    Logger& operator=(Logger&&) noexcept = default;

    template <typename... Args>
    void write_row(CSVLOG_FORMAT_STRING<Args...> fmt, Args&&... args) {
        std::lock_guard<std::mutex> lock(write_mutex);

        check_and_roll();

        auto now = std::chrono::system_clock::now();

        auto tp = std::chrono::time_point_cast<std::chrono::seconds>(now);

        std::string message =
            CSVLOG_MAKE_STRING(fmt, std::forward<Args>(args)...);

        CSVLOG_PRINT_LINE(file, "{:%F, %T}, {}", tp, message);

        current_row_count++;
        
        // # char = msglen +21 for timestamp (10+comma+space+8) and newline (1)
        current_file_size += message.length() + 21;
    }

    const std::string get_current_log_file() const {
        // std::lock_guard<std::mutex> lock(write_mutex);
        return current_filename;
    }

    void set_max_row_count(size_t count) {
        std::lock_guard<std::mutex> lock(write_mutex);
        config.max_row_count = count;
    }

    void set_max_file_size(size_t size) {
        std::lock_guard<std::mutex> lock(write_mutex);
        config.max_file_size = size;
    }

    void set_roll_time_interval(std::chrono::hours interval) {
        std::lock_guard<std::mutex> lock(write_mutex);
        config.roll_time_interval = interval;
    }
};

CSVLOG_END_NAMESPACE