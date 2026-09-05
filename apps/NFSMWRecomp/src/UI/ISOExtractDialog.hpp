#pragma once

#include <beman/cstring_view/cstring_view.hpp>

#include <rex/rex_app.h>
#include <rex/ui/imgui_dialog.h>

#include <algorithm>
#include <atomic>
#include <expected>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>

namespace NFSMW::UI {

namespace csv = beman::cstring_view;

inline constexpr std::string_view kCompleteMarker = ".extracted";

struct ExtractProgress {
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
  std::atomic<size_t> total_bytes_{0};
  std::atomic<size_t> processed_bytes_{0};
};

class ISOExtractDialog : public rex::ui::ImGuiDialog {
public:
  enum class ExtractError : std::uint8_t {
    CouldNotInitialiseDevice,
    CouldNotCreateDirectories,
    CouldNotReadSourceFile,
    CouldNotWriteDestinationFile,
    NotEnoughDiskSpace
  };

  template <typename T> using Result = std::expected<T, ExtractError>;

  explicit ISOExtractDialog(rex::ui::ImGuiDrawer *drawer,
                            std::filesystem::path game_data_root,
                            std::function<void(rex::PathConfig)> resume)
      : rex::ui::ImGuiDialog(drawer),
        game_data_root_(std::move(game_data_root)), resume_(std::move(resume)) {
  }

  [[nodiscard]] static constexpr csv::cstring_view
  to_string(ExtractError error) noexcept {
    switch (error) {
    case ExtractError::CouldNotInitialiseDevice:
      return "Could not initialise disc image device.";
    case ExtractError::CouldNotCreateDirectories:
      return "Could not create destination directories.";
    case ExtractError::CouldNotReadSourceFile:
      return "Could not read a source file from the disc image.";
    case ExtractError::CouldNotWriteDestinationFile:
      return "Could not write a file to the install directory.";
    case ExtractError::NotEnoughDiskSpace:
      return "Not enough free disk space to extract the game files.";
    default:
      return "Unknown extraction error.";
    }
  }

protected:
  void OnDraw(ImGuiIO &io) override;

private:
  struct States {
    struct SelectISO {
      std::optional<ExtractError> last_error = std::nullopt;
    };

    struct Browsing {
      std::future<std::optional<std::filesystem::path>> path_future;
    };

    struct Extracting {
      std::shared_ptr<ExtractProgress> progress;
      std::future<Result<void>> task;
    };
  };

  using State =
      std::variant<States::SelectISO, States::Browsing, States::Extracting>;

  auto Render(ImGuiIO &io) -> std::optional<bool>;
  void Update(bool select_clicked);

  [[nodiscard]] static auto PromptForISO()
      -> std::optional<std::filesystem::path>;
  auto LaunchFilePicker() -> std::future<std::optional<std::filesystem::path>>;

  [[nodiscard]] auto ExtractISO(const std::filesystem::path &iso_path,
                                ExtractProgress &progress) -> Result<void>;
  [[nodiscard]] auto
  ExtractEntryRecursive(const rex::filesystem::Entry &entry,
                        const std::filesystem::path &destination,
                        ExtractProgress &progress) -> Result<void>;
  [[nodiscard]] static auto
  ExtractFile(rex::filesystem::Entry &entry,
              const std::filesystem::path &destination_path,
              ExtractProgress &progress) -> Result<void>;

  std::filesystem::path game_data_root_;
  std::function<void(rex::PathConfig)> resume_;

  State state_ = States::SelectISO{};
};

} // namespace NFSMW::UI
