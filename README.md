# C++23 Rolling CSV Writer

A modern, header-only CSV writer class for C++23 that supports rolling files based on size, time intervals, and number of rows. Uses `std::println` for formatted output.

## Features

- **std::println Integration**: Leverages C++23's `std::println` for modern, formatted output
- **Dual Rolling Strategy**:
    - **Size-based Rolling**: Automatically creates a new log file when the current one reaches a configurable size limit
    - **Time-based Rolling**: Creates a new log file at regular intervals (e.g., daily rotation)
	- **Row-based Rolling**: Creates a new log file when the table's row count reaches a configurable threshold (excludes the header row)
- **Thread-Safe**: Uses `std::mutex` to ensure thread-safe logging operations
- **Formatted Logging**: Uses `std::format` for type-safe string formatting
- **Timestamped Rows**: Date and Time fields are added to the row by default
- **Runtime Configuration**: Change row counts, file sizes, and roll intervals at runtime
- **Header-Only**: Easy integration with just an include

## Requirements

- C++23 compatible compiler:
  - GCC 13+
  - Clang 16+
  - MSVC 2022+
- CMake 3.23+ (for building examples)

## Installation

Simply include `csvlog/logger.hpp` in your project:

```cpp
#include "csvlog/logger.hpp"
```

## Usage

### Basic Example

```cpp
#include "csvlog/logger.hpp"

int main() {
    csvlog::Logger writer;
    
	int i = 1;
	float f = 2.2f;

    writer.write_row("{},{}", i, f);
    
    return 0;
}
```

### Custom Configuration

```cpp
csvlog::Logger::Config config;
config.filename_prefix = "myapp";
config.max_row_count = 80'000;						// up to 80k rows per file
config.max_file_size = 500 * 1024 * 1024;  			// 500 MB
config.roll_time_interval = std::chrono::hours(24); // 1 day

csvlog::Logger writer(config);
```

### Formatted Logging

see [stdlog](https://github.com/5thWheel/stdlog)

### Runtime Configuration

see [stdlog](https://github.com/5thWheel/stdlog)

### Querying Current State

```cpp
std::string current_file = writer.get_current_log_file();
std::println("Currently writing to: {}", current_file);
```

## Configuration Options

### csvlog::Logger::Config

| Parameter | Default | Description |
|-----------|---------|-------------|
| `filename_prefix` | `"app"` | Prefix for CSV filenames |
| `filename_extension` | `"csv"` | Extension for filenames |
| `max_row_count` | `60'000` | Maximum number of rows in table before rolling files |
| `max_file_size` | `10 * 1024 * 1024` (10 MB) | Maximum size before rolling files |
| `roll_time_interval` | `24 hours` | Time interval for rolling files |

## File Naming

Log files are automatically named with timestamps:

```
app_20240115_143022.csv
app_20240115_150500.csv
app_20240116_000000.csv
```

This naming scheme ensures:
- Files are ordered chronologically
- Old files are easily identified and can be archived
- Multiple log files don't overwrite each other

## Record Format

Each row entry includes:
- Timestamp with second precision
- Formatted message

Example output:
```
2024-01-15, 14:30:22, 1, 2.1
2024-01-15, 14:30:23, 2, 3.14
2024-01-15, 14:30:24, 3, 5.6
```

## Thread Safety

The logger is thread-safe for concurrent logging from multiple threads. All write operations are protected by a `std::mutex`:

```cpp
std::thread t1([&logger]() {
    for (int i = 0; i < 100; ++i) {
        writer.write_row("{},{}", i, (1.1*i));
    }
});

std::thread t2([&logger]() {
    for (int i = 0; i < 100; ++i) {
        writer.write_row("{},{}", i, (2.1*i));
    }
});

t1.join();
t2.join();
```

## Performance Considerations

1. **Lock Contention**: Each log call acquires a mutex. For extremely high-frequency logging, consider batching messages.

2. **File I/O**: Files are flushed after each write. If performance is critical, consider buffering.

3. **String Formatting**: The logger uses `std::format`, which is efficient but has some overhead. Avoid excessive logging in tight loops.

4. **File Rolling**: Size checks are performed on every write, but the actual file rolling is lazy (only performed when necessary).

## License

This logger is provided as-is for educational and production use.

## Compiler Support

| Compiler | Version | Status |
|----------|---------|--------|
| GCC | 13+ | ✅ Supported |
| Clang | 16+ | ✅ Supported |
| MSVC | 2022+ | ✅ Supported |
| Apple Clang | 15+ | ✅ Supported |
