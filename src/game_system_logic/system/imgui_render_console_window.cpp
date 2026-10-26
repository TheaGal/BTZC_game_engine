#include "imgui_render_console_window.h"

#include "btlogger.h"
#include "btuuid.h"
#include "btglm.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>
#include <stdexcept>
#include <utility>


namespace
{

std::pair<ImVec4, char const*> get_log_type_repr(BT::logger::Log_type const type)
{
    switch (type)
    {
    case BT::logger::TRACE:
        return { ImVec4(0.160, 0.878, 0.890, 1), "trace" };
    case BT::logger::WARN:
        return { ImVec4(0.920, 0.880, 0.129, 1), "warn " };
    case BT::logger::ERROR:
        return { ImVec4(0.920, 0.129, 0.287, 1), "error" };
    case BT::logger::INFO:
        return { ImVec4(0.895, 0.263, 0.940, 1), "info " };
    default:
        throw std::runtime_error("get_log_type_repr() failed type");
    }
}

} // namespace


void BT::system::imgui_render_console_window()
{
    static bool s_enable_scroll_to_bottom{ true };
    static int32_t s_num_frames_scroll_away_from_bottom{ 0 };
    static logger::Log_type s_log_filter_mask{ (uint32_t)logger::ALL ^
                                               (uint32_t)logger::TRACE };
    static std::string s_log_filter_string;

    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Console viewer");
    {
        if (ImGui::Button("clear"))
            logger::clear_log_entries();

        ImGui::SameLine();
        if (ImGui::Checkbox("scroll to bottom", &s_enable_scroll_to_bottom))
            s_num_frames_scroll_away_from_bottom = 0;

        ImGui::SameLine();
        if (ImGui::Button("filter types.."))
            ImGui::OpenPopup("console_viewer_filter_popup");
        if (ImGui::BeginPopup("console_viewer_filter_popup"))
        {
            static auto const k_filter_checkbox_fn = [](std::string const& label,
                                                        logger::Log_type log_type) {
                bool enable_log_type{ (s_log_filter_mask & log_type) != 0 };
                if (ImGui::Checkbox(label.c_str(), &enable_log_type))
                {
                    if (enable_log_type)
                        s_log_filter_mask =
                            logger::Log_type((uint32_t)s_log_filter_mask | log_type);
                    else
                        s_log_filter_mask =
                            logger::Log_type((uint32_t)s_log_filter_mask ^ log_type);
                }
            };

            k_filter_checkbox_fn("enable trace logs", logger::TRACE);
            k_filter_checkbox_fn("enable warn logs", logger::WARN);
            k_filter_checkbox_fn("enable error logs", logger::ERROR);
            k_filter_checkbox_fn("enable info logs", logger::INFO);

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::InputText("search##console_viewer_search_entries", &s_log_filter_string);

        ImGui::Separator();

        // Draw log items.
        ImGui::BeginChild("Console viewer child window log items");
        {
            uint32_t start_row_idx;
            auto const log_entries = logger::read_all_log_entries(start_row_idx);

            for (uint32_t i = start_row_idx; i < log_entries.size(); i++)
            {
                auto const [log_type, mainloop_iteration, entry] = log_entries[i];

                if ((log_type & s_log_filter_mask) == 0)
                    continue;

                if (!s_log_filter_string.empty() &&
                    entry.find(s_log_filter_string) == std::string_view::npos)
                    continue;

                std::string entry_as_str(entry);

                auto const [log_type_color, log_type_str] = get_log_type_repr(log_type);

                ImGui::TextColored(log_type_color, "%s (%llu)", log_type_str, mainloop_iteration);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 1, 1, 0.5), "%s", "::");
                ImGui::SameLine();
                ImGui::TextWrapped("%s", entry_as_str.c_str());
            }

            if (s_enable_scroll_to_bottom)
            {
                ImGui::SetScrollHereY(1);
            }

            if (ImGui::GetScrollY() < ImGui::GetScrollMaxY())
            {
                s_num_frames_scroll_away_from_bottom++;
                if (s_num_frames_scroll_away_from_bottom > 3)
                    s_enable_scroll_to_bottom = false;
            }
            else
            {
                s_num_frames_scroll_away_from_bottom = 0;
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
