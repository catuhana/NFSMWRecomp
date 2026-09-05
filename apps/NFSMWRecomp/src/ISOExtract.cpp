#include "ISOExtract.hpp"

#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>
#include <rex/literals.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

#include <rex/system/xtypes.h>

using namespace rex::literals;

namespace NFSMW::ISOExtract {

namespace {

using rex::X_STATUS;

struct FileDeleter {
  void operator()(rex::filesystem::File *file) const noexcept {
    if (file != nullptr) {
      file->Destroy();
    }
  }
};

struct FileCloser {
  void operator()(FILE *file) const noexcept {
    if (file != nullptr) {
      std::fclose(file); // NOLINT(cppcoreguidelines-owning-memory)
    }
  }
};

using FilePtr = std::unique_ptr<rex::filesystem::File, FileDeleter>;
using OutputFile = std::unique_ptr<FILE, FileCloser>;

[[nodiscard]] auto ExtractFile(rex::filesystem::Entry &entry,
                               const std::filesystem::path &destination,
                               Progress &progress) -> Result<void> {
  rex::filesystem::File *raw_file = nullptr;
  if (entry.Open(rex::filesystem::FileAccess::kFileReadData, &raw_file) !=
          X_STATUS_SUCCESS ||
      raw_file == nullptr) {

    return std::unexpected(Error::CouldNotReadSourceFile);
  }

  FilePtr file(raw_file);

  auto *raw_out_file = rex::filesystem::OpenFile(destination, "wb");
  if (raw_out_file == nullptr) {
    return std::unexpected(Error::CouldNotWriteDestinationFile);
  }

  OutputFile out_file(raw_out_file);

  constexpr std::size_t kBufferSize = 4_MiB;
  std::vector<std::uint8_t> buffer(kBufferSize);

  std::size_t remaining = entry.size();
  std::size_t offset = 0;
  while (remaining > 0) {
    std::size_t bytes_read = 0;
    const std::size_t bytes_to_read = std::min(remaining, kBufferSize);

    const auto read_status =
        file->ReadSync(std::span<std::uint8_t>(buffer.data(), bytes_to_read),
                       offset, &bytes_read);
    if (read_status != X_STATUS_SUCCESS || bytes_read == 0) {
      return std::unexpected(Error::CouldNotReadSourceFile);
    }

    if (fwrite(buffer.data(), 1, bytes_read, out_file.get()) != bytes_read) {
      return std::unexpected(Error::CouldNotWriteDestinationFile);
    }

    offset += bytes_read;
    remaining -= bytes_read;
    progress.AddProcessedBytes(bytes_read);
  }

  return {};
}

[[nodiscard]] auto ExtractEntries(const rex::filesystem::Entry &entry,
                                  const std::filesystem::path &destination,
                                  Progress &progress) -> Result<void> {
  struct PendingEntry {
    const rex::filesystem::Entry *entry;
    std::filesystem::path destination;
  };

  std::vector<PendingEntry> pending_entries{
      {.entry = &entry, .destination = destination}};
  while (!pending_entries.empty()) {
    auto pending = std::move(pending_entries.back());
    pending_entries.pop_back();

    for (const auto &child : pending.entry->children()) {
      auto destination_path = pending.destination / child->name();

      if ((child->attributes() & rex::filesystem::kFileAttributeDirectory) !=
          0U) {
        std::error_code error_code;
        std::filesystem::create_directories(destination_path, error_code);
        if (error_code) {
          return std::unexpected(Error::CouldNotCreateDirectories);
        }

        pending_entries.push_back(
            {.entry = &*child, .destination = std::move(destination_path)});
      } else {
        if (auto result = ExtractFile(*child, destination_path, progress);
            !result.has_value()) {
          return result;
        }
      }
    }
  }

  return {};
}

} // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto Extract(const std::filesystem::path &iso_path,
             const std::filesystem::path &destination, Progress &progress)
    -> Result<void> {
  rex::filesystem::DiscImageDevice device("game:", iso_path);
  if (!device.Initialize()) {
    return std::unexpected(Error::CouldNotInitialiseDevice);
  }

  const auto *root = device.root();
  if (root == nullptr) {
    return std::unexpected(Error::CouldNotInitialiseDevice);
  }

  std::error_code error_code;
  std::filesystem::create_directories(destination, error_code);
  if (error_code) {
    return std::unexpected(Error::CouldNotCreateDirectories);
  }

  std::vector<const rex::filesystem::Entry *> pending_entries{root};
  while (!pending_entries.empty()) {
    const auto *entry = pending_entries.back();
    pending_entries.pop_back();

    for (const auto &child : entry->children()) {
      if ((child->attributes() & rex::filesystem::kFileAttributeDirectory) !=
          0U) {
        pending_entries.push_back(&*child);
      } else {
        progress.AddTotalBytes(child->size());
      }
    }
  }

  const auto space = std::filesystem::space(destination);
  if (space.available < progress.GetTotalBytes()) {
    return std::unexpected(Error::NotEnoughDiskSpace);
  }

  if (auto result = ExtractEntries(*root, destination, progress);
      !result.has_value()) {
    return result;
  }

  OutputFile marker(
      rex::filesystem::OpenFile(destination / kCompleteMarker, "wb"));
  if (marker == nullptr) {
    return std::unexpected(Error::CouldNotWriteDestinationFile);
  }

  return {};
}

} // namespace NFSMW::ISOExtract
