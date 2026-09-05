#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace NFSMW::ISOExtract {

inline constexpr std::string_view kCompleteMarker = ".extracted";

enum class Error : std::uint8_t {
  CouldNotInitialiseDevice,
  CouldNotCreateDirectories,
  CouldNotReadSourceFile,
  CouldNotWriteDestinationFile,
  NotEnoughDiskSpace
};

template <typename T> using Result = std::expected<T, Error>;

struct Progress {
public:
  void AddTotalBytes(std::size_t bytes) noexcept {
    total_bytes_.fetch_add(bytes, std::memory_order_relaxed);
  }

  void AddProcessedBytes(std::size_t bytes) noexcept {
    processed_bytes_.fetch_add(bytes, std::memory_order_relaxed);
  }

  [[nodiscard]] auto GetTotalBytes() const noexcept -> std::size_t {
    return total_bytes_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] auto GetProgress() const noexcept -> float {
    const std::size_t total = GetTotalBytes();
    if (total == 0) {
      return 0.0F;
    }

    const std::size_t processed =
        processed_bytes_.load(std::memory_order_relaxed);
    return std::clamp(static_cast<float>(processed) / static_cast<float>(total),
                      0.0F, 1.0F);
  }

private:
  std::atomic<std::size_t> total_bytes_{0};
  std::atomic<std::size_t> processed_bytes_{0};
};

[[nodiscard]] auto Extract(const std::filesystem::path &iso_path,
                           const std::filesystem::path &destination,
                           Progress &progress) -> Result<void>;

} // namespace NFSMW::ISOExtract
