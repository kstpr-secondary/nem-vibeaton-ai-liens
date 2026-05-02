#include "hud.h"
#include "components.h"
#include "constants.h"
#include "game.h"
#include "weapons.h"
#include "camera_rig.h"
#include <engine.h>
#include <sokol_app.h>
#include <imgui.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <cstdio>

// Bar dimensions.
static constexpr float k_bar_width  = 250.0f;
static constexpr float k_bar_height = 18.0f;

// Crosshair enemy-over indicator (set by Task 5 hit-test).
static bool s_crosshair_on_enemy = false;

// Returns false if the point is behind the camera (w <= 0 after clip).
static bool world_to_screen(const glm::vec3& world_pos,
                              const glm::mat4& vp,
                              float screen_w, float screen_h,
                              ImVec2& out_screen) {
    const glm::vec4 clip = vp * glm::vec4(world_pos, 1.f);
    if (clip.w <= 0.f)
        return false;
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    out_screen.x = (ndc.x + 1.f) * 0.5f * screen_w;
    out_screen.y = (1.f - ndc.y) * 0.5f * screen_h;  // Y flipped
    return true;
}

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
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
    ImGui::TextUnformatted("HP");
    ImGui::ProgressBar(hp_frac, ImVec2(k_bar_width, k_bar_height), "");
    ImGui::PopStyleColor();

    // Shield bar (blue)
    const float sh_frac = sh.current / sh.max;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.15f, 0.3f, 0.85f, 1.0f));
    ImGui::TextUnformatted("Shield");
    ImGui::ProgressBar(sh_frac, ImVec2(k_bar_width, k_bar_height), "");
    ImGui::PopStyleColor();

    // Boost bar (yellow)
    const float boost_frac = boost.current / boost.max;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.85f, 0.1f, 1.0f));
    ImGui::TextUnformatted("Boost");
    ImGui::ProgressBar(boost_frac, ImVec2(k_bar_width, k_bar_height), "");
    ImGui::PopStyleColor();

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

    // --- Enemy target frames ---
    {
        glm::mat4 view, proj;
        camera_rig_get_vp(view, proj);
        const glm::mat4 vp      = proj * view;
        const float     screen_w = sapp_widthf();
        const float     screen_h = sapp_heightf();

        auto& reg = engine_registry();
        auto ev = reg.view<EnemyTag, Transform, Collider, Health>();

        for (auto ee : ev) {
            const auto& et  = ev.get<Transform>(ee);
            const auto& ec  = ev.get<Collider>(ee);
            const auto& ehp = ev.get<Health>(ee);
            const auto* esh = reg.try_get<Shield>(ee);

            ImVec2 screen_center;
            if (!world_to_screen(et.position, vp, screen_w, screen_h, screen_center))
                continue;

            // Compute screen-space half-size from AABB by projecting all 8 corners.
            glm::vec3 he = ec.half_extents;
            float px_half = 0.f;
            glm::vec2 ss_min(1e9f), ss_max(-1e9f);
            for (float dx : {-he.x, he.x}) {
                for (float dy : {-he.y, he.y}) {
                    for (float dz : {-he.z, he.z}) {
                        glm::vec4 clip = vp * glm::vec4(et.position + glm::vec3(dx, dy, dz), 1.f);
                        if (clip.w > 0.f) {
                            float sx = ((clip.x / clip.w + 1.f) * 0.5f) * screen_w;
                            float sy = (1.f - clip.y / clip.w) * 0.5f * screen_h;
                            ss_min.x = std::min(ss_min.x, sx);
                            ss_max.x = std::max(ss_max.x, sx);
                            ss_min.y = std::min(ss_min.y, sy);
                            ss_max.y = std::max(ss_max.y, sy);
                        }
                    }
                }
            }
            px_half = std::max(ss_max.x - ss_min.x, ss_max.y - ss_min.y) * 0.5f;
            if (px_half < 4.f) px_half = 4.f;

            // Only draw target frame if enemy is between 3% and 20% of screen height.
            const float frac = px_half * 2.f / screen_h;
            if (frac < 0.01f || frac > 0.20f)
                continue;

            // Bracket corners.
            const float b  = px_half * 2.6f;   // bracket outer size
            const float bl = px_half * 0.8f;   // bracket leg length
            const ImU32 bc = IM_COL32(255, 220, 60, 220);
            auto* dl = ImGui::GetForegroundDrawList();

            // Top-left corner bracket
            const float x0 = screen_center.x - b, y0 = screen_center.y - b;
            const float x1 = screen_center.x + b, y1 = screen_center.y + b;
            dl->AddLine({x0,      y0},      {x0 + bl, y0},      bc, 1.5f);
            dl->AddLine({x0,      y0},      {x0,      y0 + bl}, bc, 1.5f);
            dl->AddLine({x1 - bl, y0},      {x1,      y0},      bc, 1.5f);
            dl->AddLine({x1,      y0},      {x1,      y0 + bl}, bc, 1.5f);
            dl->AddLine({x0,      y1 - bl}, {x0,      y1},      bc, 1.5f);
            dl->AddLine({x0,      y1},      {x0 + bl, y1},      bc, 1.5f);
            dl->AddLine({x1,      y1 - bl}, {x1,      y1},      bc, 1.5f);
            dl->AddLine({x1,      y1},      {x1 - bl, y1},      bc, 1.5f);

            // Health bar below bracket
            const float bar_w = b * 2.f;
            const float bar_h = 5.f;
            const float bx    = screen_center.x - b;
            const float by    = y1 + 6.f;
            const float hp_f  = ehp.current / ehp.max;
            dl->AddRectFilled({bx,          by}, {bx + bar_w,          by + bar_h},
                              IM_COL32(60, 10, 10, 180));
            dl->AddRectFilled({bx,          by}, {bx + bar_w * hp_f,   by + bar_h},
                              IM_COL32(220, 40, 40, 220));

            // Shield bar (above HP bar, if entity has Shield)
            if (esh && esh->max > 0.f) {
                const float sy   = by - bar_h - 2.f;
                const float sh_f = esh->current / esh->max;
                dl->AddRectFilled({bx,        sy}, {bx + bar_w,        sy + bar_h},
                                  IM_COL32(10, 10, 60, 180));
                dl->AddRectFilled({bx,        sy}, {bx + bar_w * sh_f, sy + bar_h},
                                  IM_COL32(60, 80, 220, 220));
            }
       }
    }

    // --- Crosshair enemy hit test ---
    {
        glm::vec3 ray_o, ray_d;
        camera_rig_cursor_ray(ray_o, ray_d);

        s_crosshair_on_enemy = false;
        auto& reg = engine_registry();
        auto ev = reg.view<EnemyTag, Transform, Collider>();
        for (auto ee : ev) {
            const auto& et = ev.get<Transform>(ee);
            const auto& ec = ev.get<Collider>(ee);

            // Ray-AABB slab test.
            const glm::vec3 inv_d = 1.f / ray_d;
            const glm::vec3 t_min = (et.position - ec.half_extents - ray_o) * inv_d;
            const glm::vec3 t_max = (et.position + ec.half_extents - ray_o) * inv_d;
            const glm::vec3 t1    = glm::min(t_min, t_max);
            const glm::vec3 t2    = glm::max(t_min, t_max);
            const float t_near    = glm::compMax(t1);
            const float t_far     = glm::compMin(t2);
            if (t_far >= t_near && t_far > 0.f) {
                s_crosshair_on_enemy = true;
                break;
            }
        }
    }

    // --- Cursor-following crosshair ---
    const glm::vec2 mpos   = engine_mouse_position();
    const float     mx     = mpos.x;
    const float     my     = mpos.y;
    auto*           dl     = ImGui::GetForegroundDrawList();

    const float ring_r     = 14.f;
    const float dot_r      = 2.5f;
    const ImU32 col_normal = IM_COL32(255, 255, 255, 210);
    const ImU32 col_enemy  = IM_COL32(255,  50,  50, 230);
    const ImU32 col        = s_crosshair_on_enemy ? col_enemy : col_normal;

    dl->AddCircle    (ImVec2(mx, my), ring_r, col, 32, 1.5f);
    dl->AddCircleFilled(ImVec2(mx, my), dot_r,  col);

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
