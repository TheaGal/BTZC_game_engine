////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Struct of settings for the app. Can be mutated and then saved to disk.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "settings.h"

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

    app_settings.renderer_settings.main_viewport_cam_orient = TXP::Renderer_settings::Camera_orientation{
        .position =
            rvec3s{
                .x = toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["position"]["x"].value_or(0.0),
                .y = toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["position"]["y"].value_or(1.0),
                .z = toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["position"]["z"].value_or(-10.0),
            },
        .facing_dir =
            vec3s{
                .x = toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["facing_dir"]["x"].value_or(0.0f),
                .y = toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["facing_dir"]["y"].value_or(0.0f),
                .z = toml_tbl["renderer_settings"]["main_viewport_cam_orient"]["facing_dir"]["z"].value_or(1.0f),
            },
    };

    app_settings.renderer_settings.open_scene_view_cam_orients.clear();
    for (size_t i = 0; i < app_settings.renderer_settings.open_scene_view_ids.size(); i++)
    {
        app_settings.renderer_settings.open_scene_view_cam_orients.emplace_back(TXP::Renderer_settings::Camera_orientation{
            .position =
                rvec3s{
                    .x = toml_tbl["renderer_settings"]["open_scene_view_cam_orients"][std::to_string(i)]["position"]["x"].value_or(0.0),
                    .y = toml_tbl["renderer_settings"]["open_scene_view_cam_orients"][std::to_string(i)]["position"]["y"].value_or(1.0),
                    .z = toml_tbl["renderer_settings"]["open_scene_view_cam_orients"][std::to_string(i)]["position"]["z"].value_or(-10.0),
                },
            .facing_dir =
                vec3s{
                    .x = toml_tbl["renderer_settings"]["open_scene_view_cam_orients"][std::to_string(i)]["facing_dir"]["x"].value_or(0.0f),
                    .y = toml_tbl["renderer_settings"]["open_scene_view_cam_orients"][std::to_string(i)]["facing_dir"]["y"].value_or(0.0f),
                    .z = toml_tbl["renderer_settings"]["open_scene_view_cam_orients"][std::to_string(i)]["facing_dir"]["z"].value_or(1.0f),
                },
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
                {
                    "position",
                    toml::table{
                        { "x", elem.position.x },
                        { "y", elem.position.y },
                        { "z", elem.position.z },
                    },
                },
                {
                    "facing_dir",
                    toml::table{
                        { "x", elem.facing_dir.x },
                        { "y", elem.facing_dir.y },
                        { "z", elem.facing_dir.z },
                    },
                },
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
                        {
                            "position",
                            toml::table{
                                { "x", app_settings.renderer_settings.main_viewport_cam_orient.position.x },
                                { "y", app_settings.renderer_settings.main_viewport_cam_orient.position.y },
                                { "z", app_settings.renderer_settings.main_viewport_cam_orient.position.z },
                            },
                        },
                        {
                            "facing_dir",
                            toml::table{
                                { "x", app_settings.renderer_settings.main_viewport_cam_orient.facing_dir.x },
                                { "y", app_settings.renderer_settings.main_viewport_cam_orient.facing_dir.y },
                                { "z", app_settings.renderer_settings.main_viewport_cam_orient.facing_dir.z },
                            },
                        },
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
