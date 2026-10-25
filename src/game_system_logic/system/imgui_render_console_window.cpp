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

    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Console viewer");
    {
        ImGui::Checkbox("scroll to bottom", &s_enable_scroll_to_bottom);

        // Draw log items.
        ImGui::BeginChild("Console viewer child window log items");
        {
            uint32_t start_row_idx;
            auto const log_entries = logger::read_all_log_entries(start_row_idx);

            for (uint32_t i = start_row_idx; i < log_entries.size(); i++)
            {
                auto const [log_type, mainloop_iteration, entry] = log_entries[i];
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
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
