#include "ISOExtractDialog.hpp"

#include <imgui.h>
#include <nfd.h>

#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/literals.h>

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

namespace NFSMW
{

  namespace UI
  {

    namespace
    {

      namespace csv = beman::cstring_view;

      using rex::X_STATUS;

      struct NFDPath
      {
        void operator()(nfdu8char_t *ptr) const noexcept
        {
          if (ptr)
            NFD_FreePathU8(ptr);
        }
      };
      using UniqueNFDPath = std::unique_ptr<nfdu8char_t, NFDPath>;

      template <class... Ts>
      struct Overload : Ts...
      {
        using Ts::operator()...;
      };

      constexpr csv::cstring_view kDescriptionText =
          "Need for Speed: Most Wanted game files were not found. Select your game ISO to extract them.";

      constexpr ImVec4 kErrorColor{1.0f, 0.35f, 0.35f, 1.0f};
      constexpr ImVec4 kProgressColor{0.00f, 0.35f, 0.00f, 1.0f};
      constexpr ImVec4 kTransparent{0.0f, 0.0f, 0.0f, 0.0f};

    }

    void ISOExtractDialog::OnDraw(ImGuiIO &io)
    {
      if (const auto select_clicked = Render(io))
        Update(*select_clicked);
    }

    std::optional<bool> ISOExtractDialog::Render(ImGuiIO &io)
    {
      ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
      ImGui::SetNextWindowSize(io.DisplaySize);

      constexpr ImGuiWindowFlags kWindowFlags = ImGuiWindowFlags_NoResize |
                                                ImGuiWindowFlags_NoMove |
                                                ImGuiWindowFlags_NoSavedSettings |
                                                ImGuiWindowFlags_NoDecoration |
                                                ImGuiWindowFlags_NoBackground;

      if (!ImGui::Begin("##ISOExtractDialog", nullptr, kWindowFlags))
      {
        ImGui::End();

        return std::nullopt;
      }

      const float em = ImGui::GetFontSize();
      const float padding_x = em * 1.25f;
      const float row_height = em * 3.8f;
      const float button_height = em * 4.2f;
      const float spacing_y = em * 1.2f;

      const float block_width = ImGui::CalcTextSize(kDescriptionText.c_str()).x + (padding_x * 2.0f);
      const float block_y = io.DisplaySize.y * 0.35f;
      const float center_x = (io.DisplaySize.x - block_width) * 0.5f;
      const float text_y_offset = (row_height - ImGui::GetTextLineHeight()) * 0.5f;

      ImGui::SetCursorPos(ImVec2(center_x, block_y));
      ImGui::BeginGroup();

      ImGui::TextUnformatted("Setup");
      ImGui::Dummy(ImVec2(0.0f, spacing_y));

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, -1.0f));

      if (ImGui::BeginChild("##Description", ImVec2(block_width, row_height), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar))
      {
        ImGui::SetCursorPos(ImVec2(padding_x, text_y_offset));
        ImGui::TextUnformatted(kDescriptionText.c_str());
      }
      ImGui::EndChild();

      if (ImGui::BeginChild("##StatusRow", ImVec2(block_width, row_height), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar))
      {
        auto draw_row = [&](csv::cstring_view label, csv::cstring_view text, bool is_error = false)
        {
          ImGui::SetCursorPos(ImVec2(padding_x, text_y_offset));
          if (is_error)
            ImGui::TextColored(kErrorColor, "%s", label.c_str());
          else
            ImGui::TextUnformatted(label.c_str());

          const float right_width = ImGui::CalcTextSize(text.c_str()).x;
          const float right_x = block_width - right_width - padding_x;
          ImGui::SameLine(right_x);

          if (is_error)
            ImGui::TextColored(kErrorColor, "%s", text.c_str());
          else
            ImGui::TextDisabled("%s", text.c_str());
        };

        std::visit(
            Overload{
                [&](const States::SelectISO &select)
                {
                  if (select.last_error)
                  {
                    draw_row("Error", to_string(*select.last_error), true);
                  }
                  else
                  {
                    std::error_code error_code;
                    const auto relative_root = std::filesystem::relative(game_data_root_, std::filesystem::current_path(), error_code);
                    const auto display_root = (!error_code && !relative_root.empty()) ? relative_root : game_data_root_;

                    draw_row("Install Directory", display_root.string());
                  }
                },
                [&](const States::Browsing &)
                { draw_row("Status", "Waiting for file picker..."); },

                [&](const States::Extracting &extracting)
                {
                  constexpr beman::cstring_view::cstring_view kExtractingLabel = "Extracting...";

                  const float progress = extracting.progress ? extracting.progress->GetProgress() : 0.0f;
                  const std::string percent_text = std::format("{:.0f}%", progress * 100.0f);

                  ImGui::PushStyleColor(ImGuiCol_FrameBg, kTransparent);
                  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, kProgressColor);
                  ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
                  ImGui::ProgressBar(progress, ImVec2(block_width, row_height), "");
                  ImGui::PopStyleColor(2);

                  ImGui::SetCursorPos(ImVec2(padding_x, text_y_offset));
                  ImGui::TextUnformatted(kExtractingLabel.c_str());
                  const float percent_width = ImGui::CalcTextSize(percent_text.c_str()).x;
                  const float percent_x = block_width - percent_width - padding_x;
                  ImGui::SetCursorPos(ImVec2(percent_x, text_y_offset));
                  ImGui::TextUnformatted(percent_text.c_str());
                }},
            state_);
      }
      ImGui::EndChild();

      ImGui::PopStyleVar();
      ImGui::Dummy(ImVec2(0.0f, spacing_y));

      const bool is_idle = std::holds_alternative<States::SelectISO>(state_);

      ImGui::BeginDisabled(!is_idle);
      const bool select_clicked = ImGui::Button("Select ISO", ImVec2(block_width, button_height));
      ImGui::EndDisabled();

      ImGui::EndGroup();
      ImGui::End();

      return select_clicked;
    }

    void ISOExtractDialog::Update(bool select_clicked)
    {
      std::optional<State> next_state;

      std::visit(
          Overload{
              [&](const States::SelectISO &)
              {
                if (select_clicked)
                  next_state = States::Browsing{.path_future = LaunchFilePicker()};
              },

              [&](States::Browsing &browsing)
              {
                if (browsing.path_future.wait_for(0s) == std::future_status::ready)
                {
                  next_state = browsing.path_future.get()
                                   .transform([this](std::filesystem::path &&path) -> State
                                              {
                                                auto progress = std::make_shared<ExtractProgress>();
                                                return States::Extracting{
                                                    .progress = progress,
                                                    .task = std::async(std::launch::async, [this, p = std::move(path), progress]()
                                                                       { return ExtractISO(p, *progress); })}; })
                                   .value_or(States::SelectISO{});
                }
              },

              [&](States::Extracting &extracting)
              {
                if (extracting.task.valid() && extracting.task.wait_for(0s) == std::future_status::ready)
                {
                  auto result = extracting.task.get();
                  if (result.has_value())
                  {
                    if (resume_)
                      resume_(rex::PathConfig{.game_data_root = game_data_root_});
                    Close();
                  }
                  else
                  {
                    next_state = States::SelectISO{.last_error = result.error()};
                  }
                }
              }},
          state_);

      if (next_state)
        state_ = std::move(*next_state);
    }

    std::future<std::optional<std::filesystem::path>> ISOExtractDialog::LaunchFilePicker()
    {
#if defined(_WIN32)
      return std::async(std::launch::async, [this]()
                        {
        NFD_Init();
        auto result = PromptForISO();
        NFD_Quit();

        return result; });
#else
      NFD_Init();
      auto result = PromptForISO();
      NFD_Quit();

      std::promise<std::optional<std::filesystem::path>> promise;
      promise.set_value(std::move(result));

      return promise.get_future();
#endif
    }

    std::optional<std::filesystem::path> ISOExtractDialog::PromptForISO()
    {
      constexpr nfdu8filteritem_t filters[] = {{"ISO Files", "iso"}};

      nfdu8char_t *raw_path = nullptr;
      const nfdopendialogu8args_t args{
          .filterList = filters,
          .filterCount = static_cast<nfdfiltersize_t>(std::size(filters))};

      if (NFD_OpenDialogU8_With(&raw_path, &args) == NFD_OKAY)
      {
        UniqueNFDPath scoped_path(raw_path);
        return std::filesystem::path(reinterpret_cast<const char *>(scoped_path.get()));
      }

      return std::nullopt;
    }

    ISOExtractDialog::Result<void> ISOExtractDialog::ExtractISO(const std::filesystem::path &iso_path,
                                                                ExtractProgress &progress)
    {
      rex::filesystem::DiscImageDevice device("game:", iso_path);
      if (!device.Initialize())
        return std::unexpected(ExtractError::CouldNotInitialiseDevice);

      std::error_code error_code;
      std::filesystem::create_directories(game_data_root_, error_code);
      if (error_code)
        return std::unexpected(ExtractError::CouldNotCreateDirectories);

      auto *root = device.root();
      if (!root)
        return std::unexpected(ExtractError::CouldNotInitialiseDevice);

      auto calculate_totals = [](auto &self, const rex::filesystem::Entry &entry, ExtractProgress &p) -> void
      {
        for (auto &child : entry.children())
        {
          if (child->attributes() & rex::filesystem::kFileAttributeDirectory)
          {
            self(self, *child, p);
          }
          else
          {
            p.total_bytes.fetch_add(child->size(), std::memory_order_relaxed);
          }
        }
      };

      calculate_totals(calculate_totals, *root, progress);

      const auto space = std::filesystem::space(game_data_root_);
      if (space.available < progress.total_bytes.load(std::memory_order_relaxed))
        return std::unexpected(ExtractError::NotEnoughDiskSpace);

      if (auto result = ExtractEntryRecursive(*root, game_data_root_, progress); !result.has_value())
        return result;

      if (auto *marker = rex::filesystem::OpenFile(game_data_root_ / kCompleteMarker, "wb"))
      {
        fclose(marker);
      }
      else
      {
        return std::unexpected(ExtractError::CouldNotWriteDestinationFile);
      }

      return {};
    }

    ISOExtractDialog::Result<void> ISOExtractDialog::ExtractEntryRecursive(const rex::filesystem::Entry &entry,
                                                                           const std::filesystem::path &destination,
                                                                           ExtractProgress &progress)
    {
      for (auto &child : entry.children())
      {
        auto destination_path = destination / child->name();

        if (child->attributes() & rex::filesystem::kFileAttributeDirectory)
        {
          std::error_code error_code;
          std::filesystem::create_directories(destination_path, error_code);
          if (error_code)
            return std::unexpected(ExtractError::CouldNotCreateDirectories);

          if (auto result = ExtractEntryRecursive(*child, destination_path, progress); !result.has_value())
            return result;

          continue;
        }

        rex::filesystem::File *raw_file = nullptr;
        if (child->Open(rex::filesystem::FileAccess::kFileReadData, &raw_file) != X_STATUS_SUCCESS || !raw_file)
          return std::unexpected(ExtractError::CouldNotReadSourceFile);

        auto file_entry = std::unique_ptr<rex::filesystem::File, void (*)(rex::filesystem::File *)>(
            raw_file, [](rex::filesystem::File *f)
            { if (f) f->Destroy(); });

        auto raw_out_file = rex::filesystem::OpenFile(destination_path, "wb");
        if (!raw_out_file)
          return std::unexpected(ExtractError::CouldNotWriteDestinationFile);

        auto out_file = std::unique_ptr<FILE, void (*)(FILE *)>(
            raw_out_file, [](FILE *f)
            { if (f) fclose(f); });

        constexpr std::size_t kBufferSize = 4_MiB;
        auto buffer = std::make_unique_for_overwrite<uint8_t[]>(kBufferSize);

        std::size_t remaining = child->size();
        std::size_t offset = 0;

        while (remaining > 0)
        {
          std::size_t bytes_read = 0;
          const std::size_t bytes_to_read = std::min(remaining, kBufferSize);

          const auto read_status =
              file_entry->ReadSync(std::span<uint8_t>(buffer.get(), bytes_to_read), offset, &bytes_read);
          if (read_status != X_STATUS_SUCCESS || bytes_read == 0)
            return std::unexpected(ExtractError::CouldNotReadSourceFile);

          if (fwrite(buffer.get(), 1, bytes_read, out_file.get()) != bytes_read)
            return std::unexpected(ExtractError::CouldNotWriteDestinationFile);

          offset += bytes_read;
          remaining -= bytes_read;

          progress.processed_bytes.fetch_add(bytes_read, std::memory_order_relaxed);
        }
      }

      return {};
    }

  }

}
