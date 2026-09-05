#pragma once

#include <beman/cstring_view/cstring_view.hpp>

#include "../ISOExtract.hpp"

#include <rex/rex_app.h>
#include <rex/ui/imgui_dialog.h>

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace NFSMW::UI::ISOExtract {

namespace csv = beman::cstring_view;

namespace Extraction = ::NFSMW::ISOExtract;

class Dialog : public rex::ui::ImGuiDialog {
public:
  explicit Dialog(rex::ui::ImGuiDrawer *drawer,
                  std::filesystem::path game_data_root,
                  std::function<void(rex::PathConfig)> resume)
      : rex::ui::ImGuiDialog(drawer),
        game_data_root_(std::move(game_data_root)), resume_(std::move(resume)) {
  }

  [[nodiscard]] static constexpr csv::cstring_view
  to_string(Extraction::Error error) noexcept {
    switch (error) {
    case Extraction::Error::CouldNotInitialiseDevice:
      return "Could not initialise disc image device.";
    case Extraction::Error::CouldNotCreateDirectories:
      return "Could not create destination directories.";
    case Extraction::Error::CouldNotReadSourceFile:
      return "Could not read a source file from the disc image.";
    case Extraction::Error::CouldNotWriteDestinationFile:
      return "Could not write a file to the install directory.";
    case Extraction::Error::NotEnoughDiskSpace:
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
      std::optional<Extraction::Error> last_error = std::nullopt;
    };

    struct Browsing {
      std::future<std::optional<std::filesystem::path>> path_future;
    };

    struct Extracting {
      std::shared_ptr<Extraction::Progress> progress;
      std::future<Extraction::Result<void>> task;
    };
  };

  using State =
      std::variant<States::SelectISO, States::Browsing, States::Extracting>;

  auto Render(ImGuiIO &io) -> std::optional<bool>;
  void Update(bool select_clicked);

  [[nodiscard]] static auto PromptForISO()
      -> std::optional<std::filesystem::path>;
  auto LaunchFilePicker() -> std::future<std::optional<std::filesystem::path>>;

  std::filesystem::path game_data_root_;
  std::function<void(rex::PathConfig)> resume_;

  State state_ = States::SelectISO{};
};

} // namespace NFSMW::UI::ISOExtract
