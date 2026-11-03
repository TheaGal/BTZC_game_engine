////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Struct of settings for the app. Can be mutated and then saved to disk.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "settings.h"

#include "btglm.h"
#include "btlogger.h"
#include "toml++/toml.hpp"
#include "txp_renderer_public.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>


namespace
{

using namespace BT;

constexpr char const* const k_settings_fname{ "settings.toml" };

static App_settings* s_app_settings{ nullptr };


namespace
{

toml::table rvec3_to_toml(rvec3s const& v)
{
    return toml::table{
        { "x", v.x },
        { "y", v.y },
        { "z", v.z },
    };
}

toml::table vec3_to_toml(vec3s const& v)
{
    return rvec3_to_toml(rvec3s{ v.x, v.y, v.z });
}

rvec3s toml_to_rvec3(toml::node_view<toml::node const> const& view, rvec3s const& default_v)
{
    return rvec3s{
        .x = view["x"].value_or(default_v.x),
        .y = view["y"].value_or(default_v.y),
        .z = view["z"].value_or(default_v.z),
    };
}

vec3s toml_to_vec3(toml::node_view<toml::node const> const& view, vec3s const& default_v)
{
    rvec3s ret = toml_to_rvec3(view, rvec3s{ default_v.x, default_v.y, default_v.z });
    return vec3s{ static_cast<float_t>(ret.x),
                  static_cast<float_t>(ret.y),
                  static_cast<float_t>(ret.z) };
}

} // namespace


/// Converts TOML file contents to app settings, only changing values that exist in the file.
void toml_to_app_settings(toml::table const& toml_tbl, App_settings& app_settings)
{
    app_settings.renderer_settings.monitor_idx     = toml_tbl["renderer_settings"]["monitor_idx"].value_or(app_settings.renderer_settings.monitor_idx);
    app_settings.renderer_settings.windowed_width  = toml_tbl["renderer_settings"]["windowed_width"].value_or(app_settings.renderer_settings.windowed_width);
    app_settings.renderer_settings.windowed_height = toml_tbl["renderer_settings"]["windowed_height"].value_or(app_settings.renderer_settings.windowed_height);
    app_settings.renderer_settings.is_vsync_on     = toml_tbl["renderer_settings"]["is_vsync_on"].value_or(app_settings.renderer_settings.is_vsync_on);
    app_settings.renderer_settings.is_resizable    = toml_tbl["renderer_settings"]["is_resizable"].value_or(app_settings.renderer_settings.is_resizable);
    app_settings.renderer_settings.has_border      = toml_tbl["renderer_settings"]["has_border"].value_or(app_settings.renderer_settings.has_border);
    app_settings.renderer_settings.is_fullscreen   = toml_tbl["renderer_settings"]["is_fullscreen"].value_or(app_settings.renderer_settings.is_fullscreen);

    app_settings.renderer_settings.open_scene_view_ids.clear();
    for (auto const& elem : *toml_tbl["renderer_settings"]["open_scene_view_ids"].as_array())
    {
        app_settings.renderer_settings.open_scene_view_ids.emplace_back(elem.value_or("unknown"));
    }

    app_settings.renderer_settings.main_viewport_cam_orient =
        TXP::Renderer_settings::Camera_orientation{
            .position =
                toml_to_rvec3(toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["position"],
                              rvec3s{ 0, 1, -10 }),
            .facing_dir = toml_to_vec3(
                toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["facing_dir"],
                vec3s{ 0, 0, 1 }),
        };

    app_settings.renderer_settings.open_scene_view_cam_orients.clear();
    for (size_t i = 0; i < app_settings.renderer_settings.open_scene_view_ids.size(); i++)
    {
        app_settings.renderer_settings.open_scene_view_cam_orients.emplace_back(
            TXP::Renderer_settings::Camera_orientation{
                .position =
                    toml_to_rvec3(toml_tbl["renderer_settings"]["open_scene_view_cam_orients"]
                                          [std::to_string(i)]["position"],
                                  rvec3s{ 0, 1, -10 }),
                .facing_dir =
                    toml_to_vec3(toml_tbl["renderer_settings"]["open_scene_view_cam_orients"]
                                         [std::to_string(i)]["facing_dir"],
                                 vec3s{ 0, 0, 1 }),
            });
    }
}

/// Converts app settings into TOML file, including all fields. Returns the TOML file.
toml::table app_settings_to_toml(App_settings& app_settings)
{
    toml::array open_scene_view_ids_arr;
    for (auto const& elem : app_settings.renderer_settings.open_scene_view_ids)
        open_scene_view_ids_arr.emplace_back(elem);

    toml::table open_scene_view_cam_orients_tbl;
    for (auto const& elem : app_settings.renderer_settings.open_scene_view_cam_orients)
    {
        open_scene_view_cam_orients_tbl.emplace(
            std::to_string(open_scene_view_cam_orients_tbl.size()),
            toml::table{
                { "position", rvec3_to_toml(elem.position) },
                { "facing_dir", vec3_to_toml(elem.facing_dir) },
            });
    }

    return toml::table{
        {
            "renderer_settings",
            toml::table{
                { "monitor_idx", app_settings.renderer_settings.monitor_idx },
                { "windowed_width", app_settings.renderer_settings.windowed_width },
                { "windowed_height", app_settings.renderer_settings.windowed_height },
                { "is_vsync_on", app_settings.renderer_settings.is_vsync_on },
                { "is_resizable", app_settings.renderer_settings.is_resizable },
                { "has_border", app_settings.renderer_settings.has_border },
                { "is_fullscreen", app_settings.renderer_settings.is_fullscreen },
                { "open_scene_view_ids", open_scene_view_ids_arr },
                {
                    "main_viewport_cam_orient",
                    toml::table{
                        { "position",
                          rvec3_to_toml(
                              app_settings.renderer_settings.main_viewport_cam_orient.position) },
                        { "facing_dir",
                          vec3_to_toml(
                              app_settings.renderer_settings.main_viewport_cam_orient.facing_dir) },
                    },
                },
                { "open_scene_view_cam_orients", open_scene_view_cam_orients_tbl },
            },
        },
    };
}

}  // namespace


void BT::App_settings::send_settings_to_renderer(TXP::Renderer& renderer) const
{
    renderer.send_new_settings(renderer_settings);
}

void BT::App_settings::load_settings_from_renderer(TXP::Renderer& renderer)
{
    renderer_settings = renderer.get_current_settings();
}

void BT::initialize_app_settings_from_file_or_fallback_to_defaults()
{   // Ensure doesn't run multiple times.
    assert(s_app_settings == nullptr);

    // Create default app settings.
    static App_settings s_main_app_settings;
    s_app_settings = &s_main_app_settings;

    // Try to load app settings from a file.
    if (std::filesystem::exists(k_settings_fname) &&
        std::filesystem::is_regular_file(k_settings_fname))
    {
        try
        {   // Load settings from disk and deserialize.
            toml::table tbl{ toml::parse_file(k_settings_fname) };
            toml_to_app_settings(tbl, s_main_app_settings);

            BT_TRACEF("Successfully loaded app settings from \"%s\"", k_settings_fname);
        }
        catch (toml::parse_error const& err)
        {
            BT_ERRORF("Parsing failed of file\"%s\": %s", k_settings_fname, err.what());
        }
    }
    else
    {   // Write default settings to disk.
        save_app_settings_to_disk();
    }
}

BT::App_settings const& BT::get_app_settings_read_handle()
{
    assert(s_app_settings != nullptr);
    return *s_app_settings;
}

BT::App_settings& BT::get_app_settings_write_handle()
{
    assert(s_app_settings != nullptr);
    return *s_app_settings;
}

void BT::save_app_settings_to_disk()
{
    auto toml_tbl{ app_settings_to_toml(*s_app_settings) };

    // Write to file.
    std::ofstream f{ k_settings_fname };
    f << toml_tbl << "\n";

    BT_TRACEF("Successfully wrote app settings to \"%s\"", k_settings_fname);
}
