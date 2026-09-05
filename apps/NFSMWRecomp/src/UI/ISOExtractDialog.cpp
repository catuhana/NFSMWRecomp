#include "ISOExtractDialog.hpp"

#include <imgui.h>
#include <nfd.hpp>

#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/literals.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>

#include <rex/system/xtypes.h>

using namespace std::chrono_literals;
using namespace rex::literals;

namespace NFSMW::UI {

namespace csv = beman::cstring_view;

using rex::X_STATUS;

namespace {

template <class... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};

constexpr csv::cstring_view kDescriptionText =
    "Need for Speed: Most Wanted game files were not found. Select your game "
    "ISO to extract them.";

constexpr ImVec4 kErrorColor{1.0F, 0.35F, 0.35F, 1.0F};
constexpr ImVec4 kProgressColor{0.00F, 0.35F, 0.00F, 1.0F};
constexpr ImVec4 kTransparent{0.0F, 0.0F, 0.0F, 0.0F};

} // namespace

void ISOExtractDialog::OnDraw(ImGuiIO &io) {
  if (const auto select_clicked = Render(io)) {
    Update(*select_clicked);
  }
}

auto ISOExtractDialog::Render(ImGuiIO &io) -> std::optional<bool> {
  ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
  ImGui::SetNextWindowSize(io.DisplaySize);

  constexpr ImGuiWindowFlags kWindowFlags =
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
      ImGuiWindowFlags_NoBackground;

  if (!ImGui::Begin("##ISOExtractDialog", nullptr, kWindowFlags)) {
    ImGui::End();

    return std::nullopt;
  }

  const float em = ImGui::GetFontSize();
  const float padding_x = em * 1.25F;
  const float row_height = em * 3.8F;
  const float button_height = em * 4.2F;
  const float spacing_y = em * 1.2F;

  const float block_width =
      ImGui::CalcTextSize(kDescriptionText.c_str()).x + (padding_x * 2.0F);
  const float block_y = io.DisplaySize.y * 0.35F;
  const float center_x = (io.DisplaySize.x - block_width) * 0.5F;
  const float text_y_offset = (row_height - ImGui::GetTextLineHeight()) * 0.5F;

  ImGui::SetCursorPos(ImVec2(center_x, block_y));
  ImGui::BeginGroup();

  ImGui::TextUnformatted("Setup");
  ImGui::Dummy(ImVec2(0.0F, spacing_y));

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0F, -1.0F));

  if (ImGui::BeginChild("##Description", ImVec2(block_width, row_height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar)) {
    ImGui::SetCursorPos(ImVec2(padding_x, text_y_offset));
    ImGui::TextUnformatted(kDescriptionText.c_str());
  }
  ImGui::EndChild();

  if (ImGui::BeginChild("##StatusRow", ImVec2(block_width, row_height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar)) {
    auto draw_row = [&](csv::cstring_view label, csv::cstring_view text,
                        bool is_error = false) {
      ImGui::SetCursorPos(ImVec2(padding_x, text_y_offset));
      if (is_error) {
        ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopStyleColor();
      } else {
        ImGui::TextUnformatted(label.c_str());
      }

      const float right_width = ImGui::CalcTextSize(text.c_str()).x;
      const float right_x = block_width - right_width - padding_x;
      ImGui::SameLine(right_x);

      if (is_error) {
        ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopStyleColor();
      } else {
        ImGui::TextUnformatted(text.c_str());
      }
    };

    std::visit(
        Overload{
            [&](const States::SelectISO &select) {
              if (select.last_error) {
                draw_row("Error", to_string(*select.last_error), true);
              } else {
                std::error_code error_code;
                const auto relative_root = std::filesystem::relative(
                    game_data_root_, std::filesystem::current_path(),
                    error_code);
                const auto display_root =
                    (!error_code && !relative_root.empty()) ? relative_root
                                                            : game_data_root_;

                draw_row("Install Directory", display_root.string());
              }
            },
            [&](const States::Browsing &) {
              draw_row("Status", "Waiting for file picker...");
            },

            [&](const States::Extracting &extracting) {
              constexpr beman::cstring_view::cstring_view kExtractingLabel =
                  "Extracting...";

              const float progress = extracting.progress
                                         ? extracting.progress->GetProgress()
                                         : 0.0F;
              const std::string percent_text =
                  std::format("{:.0f}%", progress * 100.0F);

              ImGui::PushStyleColor(ImGuiCol_FrameBg, kTransparent);
              ImGui::PushStyleColor(ImGuiCol_PlotHistogram, kProgressColor);
              ImGui::SetCursorPos(ImVec2(0.0F, 0.0F));
              ImGui::ProgressBar(progress, ImVec2(block_width, row_height), "");
              ImGui::PopStyleColor(2);

              ImGui::SetCursorPos(ImVec2(padding_x, text_y_offset));
              ImGui::TextUnformatted(kExtractingLabel.c_str());
              const float percent_width =
                  ImGui::CalcTextSize(percent_text.c_str()).x;
              const float percent_x = block_width - percent_width - padding_x;
              ImGui::SetCursorPos(ImVec2(percent_x, text_y_offset));
              ImGui::TextUnformatted(percent_text.c_str());
            }},
        state_);
  }
  ImGui::EndChild();

  ImGui::PopStyleVar();
  ImGui::Dummy(ImVec2(0.0F, spacing_y));

  const bool is_idle = std::holds_alternative<States::SelectISO>(state_);

  ImGui::BeginDisabled(!is_idle);
  const bool select_clicked =
      ImGui::Button("Select ISO", ImVec2(block_width, button_height));
  ImGui::EndDisabled();

  ImGui::EndGroup();
  ImGui::End();

  return select_clicked;
}

void ISOExtractDialog::Update(bool select_clicked) {
  std::optional<State> next_state;

  std::visit(
      Overload{
          [&](const States::SelectISO &) {
            if (select_clicked) {
              next_state = States::Browsing{.path_future = LaunchFilePicker()};
            }
          },

          [&](States::Browsing &browsing) {
            if (browsing.path_future.wait_for(0s) ==
                std::future_status::ready) {
              next_state =
                  browsing.path_future.get()
                      .transform([this](std::filesystem::path &&path) -> State {
                        auto progress = std::make_shared<ExtractProgress>();
                        return States::Extracting{
                            .progress = progress,
                            .task = std::async(
                                std::launch::async,
                                [this, path = std::move(path), progress]() {
                                  return ExtractISO(path, *progress);
                                })};
                      })
                      .value_or(States::SelectISO{});
            }
          },

          [&](States::Extracting &extracting) {
            if (extracting.task.valid() &&
                extracting.task.wait_for(0s) == std::future_status::ready) {
              auto result = extracting.task.get();
              if (result.has_value()) {
                if (resume_) {
                  resume_(rex::PathConfig{.game_data_root = game_data_root_});
                }
                Close();
              } else {
                next_state = States::SelectISO{.last_error = result.error()};
              }
            }
          }},
      state_);

  if (next_state) {
    state_ = std::move(*next_state);
  }
}

auto ISOExtractDialog::LaunchFilePicker()
    -> std::future<std::optional<std::filesystem::path>> {
#if defined(_WIN32)
  return std::async(std::launch::async, [this]() { return PromptForISO(); });
#else
  auto result = PromptForISO();

  std::promise<std::optional<std::filesystem::path>> promise;
  promise.set_value(std::move(result));

  return promise.get_future();
#endif
}

auto ISOExtractDialog::PromptForISO() -> std::optional<std::filesystem::path> {
  NFD::Guard nfd_guard;
  NFD::UniquePath out_path;

  constexpr std::array<nfdfilteritem_t, 1> filters = {{"ISO Files", "iso"}};

  if (NFD::OpenDialog(out_path, filters.data(),
                      static_cast<nfdfiltersize_t>(filters.size())) ==
      NFD_OKAY) {
    return std::filesystem::path(out_path.get());
  }

  return std::nullopt;
}

auto ISOExtractDialog::ExtractISO(const std::filesystem::path &iso_path,
                                  ExtractProgress &progress)
    -> ISOExtractDialog::Result<void> {
  rex::filesystem::DiscImageDevice device("game:", iso_path);
  if (!device.Initialize()) {
    return std::unexpected(ExtractError::CouldNotInitialiseDevice);
  }

  std::error_code error_code;
  std::filesystem::create_directories(game_data_root_, error_code);
  if (error_code) {
    return std::unexpected(ExtractError::CouldNotCreateDirectories);
  }

  const auto *root = device.root();
  if (root == nullptr) {
    return std::unexpected(ExtractError::CouldNotInitialiseDevice);
  }

  auto calculate_totals = [](this auto &self,
                             const rex::filesystem::Entry &entry,
                             ExtractProgress &progress) -> void {
    for (const auto &child : entry.children()) {
      if (child->attributes() & rex::filesystem::kFileAttributeDirectory) {
        self(*child, progress);
      } else {
        progress.AddTotalBytes(child->size());
      }
    }
  };

  calculate_totals(*root, progress);

  const auto space = std::filesystem::space(game_data_root_);
  if (space.available < progress.GetTotalBytes()) {
    return std::unexpected(ExtractError::NotEnoughDiskSpace);
  }

  if (auto result = ExtractEntryRecursive(*root, game_data_root_, progress);
      !result.has_value()) {
    return result;
  }

  if (auto *marker =
          rex::filesystem::OpenFile(game_data_root_ / kCompleteMarker, "wb")) {
    fclose(marker); // NOLINT
  } else {
    return std::unexpected(ExtractError::CouldNotWriteDestinationFile);
  }

  return {};
}

auto ISOExtractDialog::ExtractEntryRecursive(
    const rex::filesystem::Entry &entry,
    const std::filesystem::path &destination, ExtractProgress &progress)
    -> ISOExtractDialog::Result<void> {
  for (const auto &child : entry.children()) {
    auto destination_path = destination / child->name();

    if ((child->attributes() & rex::filesystem::kFileAttributeDirectory) !=
        0U) {
      std::error_code error_code;
      std::filesystem::create_directories(destination_path, error_code);
      if (error_code) {
        return std::unexpected(ExtractError::CouldNotCreateDirectories);
      }

      if (auto result =
              ExtractEntryRecursive(*child, destination_path, progress);
          !result.has_value()) {
        return result;
      }

      continue;
    }

    if (auto result = ExtractFile(*child, destination_path, progress);
        !result.has_value()) {
      return result;
    }
  }

  return {};
}

auto ISOExtractDialog::ExtractFile(
    rex::filesystem::Entry &entry,
    const std::filesystem::path &destination_path, ExtractProgress &progress)
    -> ISOExtractDialog::Result<void> {
  rex::filesystem::File *raw_file = nullptr;
  if (entry.Open(rex::filesystem::FileAccess::kFileReadData, &raw_file) !=
          X_STATUS_SUCCESS ||
      (raw_file == nullptr)) {
    return std::unexpected(ExtractError::CouldNotReadSourceFile);
  }

  auto file_entry =
      std::unique_ptr<rex::filesystem::File, void (*)(rex::filesystem::File *)>(
          raw_file, [](rex::filesystem::File *file) {
            if (file) {
              file->Destroy();
            }
          });

  auto *raw_out_file = rex::filesystem::OpenFile(destination_path, "wb");
  if (raw_out_file == nullptr) {
    return std::unexpected(ExtractError::CouldNotWriteDestinationFile);
  }

  std::unique_ptr<FILE, decltype(&fclose)> out_file(raw_out_file, &fclose);

  constexpr std::size_t kBufferSize = 4_MiB;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
  auto buffer = std::make_unique_for_overwrite<uint8_t[]>(kBufferSize);

  std::size_t remaining = entry.size();
  std::size_t offset = 0;

  while (remaining > 0) {
    std::size_t bytes_read = 0;
    const std::size_t bytes_to_read = std::min(remaining, kBufferSize);

    const auto read_status = file_entry->ReadSync(
        std::span<uint8_t>(buffer.get(), bytes_to_read), offset, &bytes_read);
    if (read_status != X_STATUS_SUCCESS || bytes_read == 0) {
      return std::unexpected(ExtractError::CouldNotReadSourceFile);
    }

    if (fwrite(buffer.get(), 1, bytes_read, out_file.get()) != bytes_read) {
      return std::unexpected(ExtractError::CouldNotWriteDestinationFile);
    }

    offset += bytes_read;
    remaining -= bytes_read;

    progress.AddProcessedBytes(bytes_read);
  }

  return {};
}

} // namespace NFSMW::UI
