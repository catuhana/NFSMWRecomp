#include "ISOExtractDialog.hpp"

#include <imgui.h>
#include <nfd.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

using namespace std::chrono_literals;

namespace NFSMW::UI::ISOExtract {

namespace csv = beman::cstring_view;

namespace {

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

constexpr csv::cstring_view kDescriptionText =
    "Need for Speed: Most Wanted game files were not found. Select your game "
    "ISO to extract them.";

constexpr ImVec4 kErrorColor{1.0F, 0.35F, 0.35F, 1.0F};
constexpr ImVec4 kProgressColor{0.00F, 0.35F, 0.00F, 1.0F};
constexpr ImVec4 kTransparent{0.0F, 0.0F, 0.0F, 0.0F};

} // namespace

void Dialog::OnDraw(ImGuiIO &io) {
  if (const auto select_clicked = Render(io)) {
    Update(*select_clicked);
  }
}

auto Dialog::Render(ImGuiIO &io) -> std::optional<bool> {
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

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0F, -1.0F));

  ImGui::TextUnformatted("Setup");
  ImGui::Dummy(ImVec2(0.0F, spacing_y));

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
        Overloaded{
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

void Dialog::Update(bool select_clicked) {
  std::optional<State> next_state;

  std::visit(
      Overloaded{
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
                        auto progress =
                            std::make_shared<Extraction::Progress>();

                        return States::Extracting{
                            .progress = progress,
                            .task = std::async(
                                std::launch::async,
                                [this, path = std::move(path), progress]() {
                                  return Extraction::Extract(
                                      path, game_data_root_, *progress);
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

auto Dialog::LaunchFilePicker()
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

auto Dialog::PromptForISO() -> std::optional<std::filesystem::path> {
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

} // namespace NFSMW::UI::ISOExtract
