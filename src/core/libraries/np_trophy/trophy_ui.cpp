// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <mutex>
#include <imgui.h>
#include "common/assert.h"
#include "common/singleton.h"
#include "imgui/imgui_std.h"
#include "trophy_ui.h"

using namespace ImGui;

namespace Libraries::NpTrophy {

std::optional<TrophyUI> current_trophy_ui;
std::queue<TrophyInfo> trophy_queue;
std::mutex queueMtx;

TrophyUI::TrophyUI(const std::filesystem::path& trophyIconPath, const std::string& trophyName)
    : trophy_name(trophyName) {
    if (std::filesystem::exists(trophyIconPath)) {
        trophy_icon = RefCountedTexture::DecodePngFile(trophyIconPath);
    } else {
        LOG_ERROR(Lib_NpTrophy, "Couldn't load trophy icon at {}",
                  fmt::UTF(trophyIconPath.u8string()));
    }
    AddLayer(this);
}

TrophyUI::~TrophyUI() {
    Finish();
}

void TrophyUI::Finish() {
    RemoveLayer(this);
}

void TrophyUI::Draw() {
    const auto& io = GetIO();

    // Window dimensions and position
    const float window_width = 300.0f; // Slightly wider for better layout
    const float window_height = 80.0f; // More space for icon and text

    const ImVec2 window_size(window_width, window_height);
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    SetNextWindowPos(ImVec2(io.DisplaySize.x - window_width - 10, 50), ImGuiCond_Always);

    if (Begin("Trophy Window", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground)) {
        
        // Align the icon and text in a horizontal layout
        if (trophy_icon) {
            ImGui::SetCursorPos(ImVec2(10, 15)); // Padding from the top-left corner
            Image(trophy_icon.GetTexture().im_id, ImVec2(50, 50)); // Icon size (50x50)
            ImGui::SameLine();
        } else {
            // Render a placeholder box when the icon isn't available
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{50.0f, 50.0f},
                                                      GetColorU32(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}));
            ImGui::SetCursorPosX(60);  // Indent text if no icon is present
        }

        // Adjust text layout with padding
        ImGui::SetCursorPosY(20);
        TextWrapped("Trophy Earned!\n%s", trophy_name.c_str());
    }
    End();

    // Manage trophy display timer and queue
    trophy_timer -= io.DeltaTime;
    if (trophy_timer <= 0) {
        std::lock_guard<std::mutex> lock(queueMtx);
        if (!trophy_queue.empty()) {
            TrophyInfo next_trophy = trophy_queue.front();
            trophy_queue.pop();
            current_trophy_ui.emplace(next_trophy.trophy_icon_path, next_trophy.trophy_name);
        } else {
            current_trophy_ui.reset();
        }
    }
}

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName) {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (current_trophy_ui.has_value()) {
        TrophyInfo new_trophy;
        new_trophy.trophy_icon_path = trophyIconPath;
        new_trophy.trophy_name = trophyName;
        trophy_queue.push(new_trophy);
    } else {
        current_trophy_ui.emplace(trophyIconPath, trophyName);
    }
}

} // namespace Libraries::NpTrophy
