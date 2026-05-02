#include "hud.h"
#include "components.h"
#include "constants.h"
#include "game.h"
#include "weapons.h"
#include <engine.h>
#include <sokol_app.h>
#include <imgui.h>
#include <cstdio>

// Bar dimensions.
static constexpr float k_bar_width  = 250.0f;
static constexpr float k_bar_height = 18.0f;

void hud_render() {
    auto& reg = engine_registry();
    auto player_view = reg.view<PlayerTag, Health, Shield, Boost, WeaponState>();
    if (player_view.begin() == player_view.end())
        return;

    const entt::entity player_e = *player_view.begin();
    const auto& hp              = player_view.get<Health>(player_e);
    const auto& sh              = player_view.get<Shield>(player_e);
    const auto& boost           = player_view.get<Boost>(player_e);
    const auto& ws              = player_view.get<WeaponState>(player_e);

    // --- Resource bars (top-left corner) ---
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(k_bar_width + 40, 120));
    ImGui::Begin("##hud_bars", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoInputs);

    // HP bar (red)
    const float hp_frac = hp.current / hp.max;
    ImGui::TextUnformatted("HP");
    ImGui::ProgressBar(hp_frac, ImVec2(k_bar_width, k_bar_height), "");
    ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8f, 0.15f, 0.15f, 1.0f);

    // Shield bar (blue)
    const float sh_frac = sh.current / sh.max;
    ImGui::TextUnformatted("Shield");
    ImGui::ProgressBar(sh_frac, ImVec2(k_bar_width, k_bar_height), "");
    ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = ImVec4(0.15f, 0.3f, 0.85f, 1.0f);

    // Boost bar (yellow)
    const float boost_frac = boost.current / boost.max;
    ImGui::TextUnformatted("Boost");
    ImGui::ProgressBar(boost_frac, ImVec2(k_bar_width, k_bar_height), "");
    ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = ImVec4(0.9f, 0.85f, 0.1f, 1.0f);

    ImGui::End();

    // --- Weapon info (top-right corner) ---
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(sapp_width()) - 220, 10));
    ImGui::SetNextWindowSize(ImVec2(210, 80));
    ImGui::Begin("##hud_weapon", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoInputs);

    const char* weapon_name = (ws.active_weapon == WeaponType::Laser) ? "Laser" : "Plasma";
    ImGui::Text("Weapon: %s", weapon_name);

    // Cooldown display for active weapon.
    const double cd_max = (ws.active_weapon == WeaponType::Laser)
        ? static_cast<double>(ws.laser_cooldown)
        : static_cast<double>(ws.plasma_cooldown);
    const double last_fire = (ws.active_weapon == WeaponType::Laser)
        ? ws.laser_last_fire
        : ws.plasma_last_fire;
    const double elapsed   = engine_now() - last_fire;
    const float  cd_frac   = static_cast<float>(elapsed / cd_max);
    const bool   ready     = cd_frac >= 1.0f;

    if (ready) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "READY");
    } else {
        ImGui::Text("Cooldown: %.1fs", cd_max - elapsed);
        ImGui::ProgressBar(cd_frac, ImVec2(k_bar_width * 0.6f, k_bar_height * 0.6f), "");
    }

    ImGui::End();

   // --- Centered crosshair (foreground, always on top) ---
    auto* dl = ImGui::GetForegroundDrawList();
    const float cx = sapp_width()  * 0.5f;
    const float cy = sapp_height() * 0.5f;
    const float arm = 10.f;
    const float gap = 3.f;
    ImU32 col = IM_COL32(255, 255, 255, 200);
    dl->AddLine({cx - arm - gap, cy}, {cx - gap, cy}, col, 1.5f);
    dl->AddLine({cx + gap, cy}, {cx + arm + gap, cy}, col, 1.5f);
    dl->AddLine({cx, cy - arm - gap}, {cx, cy - gap}, col, 1.5f);
    dl->AddLine({cx, cy + gap}, {cx, cy + arm + gap}, col, 1.5f);

    // --- Terminal overlays (win / death) ---
    const MatchState& ms = get_match_state();
    if (ms.phase == MatchPhase::PlayerDead || ms.phase == MatchPhase::Victory) {
        const bool is_victory = (ms.phase == MatchPhase::Victory);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(sapp_width()),
                                         static_cast<float>(sapp_height())));
        ImGui::Begin("##overlay", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoInputs);

        // Semi-transparent dark backdrop.
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(ImVec2(0, 0),
                            ImVec2(static_cast<float>(sapp_width()),
                                   static_cast<float>(sapp_height())),
                            IM_COL32(0, 0, 0, 160));

        // Centered text.
        const float font_size = 60.0f;
        const char* title = is_victory ? "YOU WIN" : "YOU DIED";

        const float tw = ImGui::CalcTextSize(title, nullptr, false, font_size).x;
        const float tx = (static_cast<float>(sapp_width())  - tw) * 0.5f;
        const float ty = static_cast<float>(sapp_height()) * 0.4f;

        ImGui::PushFont(nullptr);
        ImGui::SetCursorPos(ImVec2(tx, ty));
        ImGui::PushTextWrapPos(static_cast<float>(sapp_width()));
        ImGui::TextColored(ImVec4(
            is_victory ? 1.0f : 0.86f,
            is_victory ? 0.84f : 0.16f,
            is_victory ? 0.0f : 0.16f,
            1.0f),
            "%s", title);
        ImGui::PopTextWrapPos();
        ImGui::PopFont();

        // Subtitle prompt.
        const char* subtitle = "Press Enter to restart";
        const float sw = ImGui::CalcTextSize(subtitle).x;
        const float sx = (static_cast<float>(sapp_width()) - sw) * 0.5f;

        ImGui::SetCursorPos(ImVec2(sx, ty + font_size + 20.0f));
        ImGui::TextUnformatted(subtitle);

        // Auto-restart countdown.
        const double elapsed = engine_now() - ms.phase_enter_time;
        const double remaining = ms.auto_restart_delay - elapsed;
        if (remaining > 0.0) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "Auto-restart in %.0fs", remaining);
            const float cw = ImGui::CalcTextSize(buf).x;
            const float cx = (static_cast<float>(sapp_width()) - cw) * 0.5f;
            ImGui::SetCursorPos(ImVec2(cx, ty + font_size + 50.0f));
            ImGui::TextUnformatted(buf);
        }

        ImGui::End();
    }
}
