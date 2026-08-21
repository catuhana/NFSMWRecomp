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

namespace NFSMW
{

  namespace UI
  {

    namespace csv = beman::cstring_view;

    inline constexpr std::string_view kCompleteMarker = ".extracted";

    struct ExtractProgress
    {
      std::atomic<size_t> total_bytes{0};
      std::atomic<size_t> processed_bytes{0};

      [[nodiscard]] float GetProgress() const noexcept
      {
        const size_t total = total_bytes.load(std::memory_order_relaxed);
        if (total == 0)
          return 0.0f;

        const size_t processed = processed_bytes.load(std::memory_order_relaxed);
        return std::clamp(static_cast<float>(processed) / static_cast<float>(total), 0.0f, 1.0f);
      }
    };

    class ISOExtractDialog : public rex::ui::ImGuiDialog
    {
    public:
      enum class ExtractError
      {
        CouldNotInitialiseDevice,
        CouldNotCreateDirectories,
        CouldNotReadSourceFile,
        CouldNotWriteDestinationFile,
        NotEnoughDiskSpace
      };

      template <typename T>
      using Result = std::expected<T, ExtractError>;

      explicit ISOExtractDialog(rex::ui::ImGuiDrawer *drawer,
                                std::filesystem::path game_data_root,
                                std::function<void(rex::PathConfig)> resume)
          : rex::ui::ImGuiDialog(drawer),
            game_data_root_(std::move(game_data_root)),
            resume_(std::move(resume))
      {
      }

      [[nodiscard]] static constexpr csv::cstring_view to_string(ExtractError error) noexcept
      {
        switch (error)
        {
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
      struct States
      {
        struct SelectISO
        {
          std::optional<ExtractError> last_error = std::nullopt;
        };

        struct Browsing
        {
          std::future<std::optional<std::filesystem::path>> path_future;
        };

        struct Extracting
        {
          std::shared_ptr<ExtractProgress> progress;
          std::future<Result<void>> task;
        };
      };

      using State = std::variant<States::SelectISO, States::Browsing, States::Extracting>;

      std::optional<bool> Render(ImGuiIO &io);
      void Update(bool select_clicked);

      [[nodiscard]] std::optional<std::filesystem::path> PromptForISO();
      std::future<std::optional<std::filesystem::path>> LaunchFilePicker();

      [[nodiscard]] Result<void> ExtractISO(const std::filesystem::path &iso_path,
                                            ExtractProgress &progress);
      [[nodiscard]] Result<void> ExtractEntryRecursive(const rex::filesystem::Entry &entry,
                                                       const std::filesystem::path &destination,
                                                       ExtractProgress &progress);

      std::filesystem::path game_data_root_;
      std::function<void(rex::PathConfig)> resume_;

      State state_ = States::SelectISO{};
    };

  }

}
