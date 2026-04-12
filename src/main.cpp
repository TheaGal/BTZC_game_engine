#include "audio_engine/audio_engine.h"
#include "btuuid.h"
#include "btzc_game_engine.h"
#include "btglm.h"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/_dev_animation_frame_action_editor.h"
#include "game_system_logic/system/animator_driven_hitcapsule_sets_update.h"
#include "game_system_logic/system/character_broadcast_attack_msg_to_enemies.h"
#include "game_system_logic/system/cpu_character_enemy_detection.h"
#include "game_system_logic/system/cpu_character_world_space_input.h"
#include "game_system_logic/system/hitcapsule_attack_processing.h"
#include "game_system_logic/system/imgui_render_transform_hierarchy_window.h"
#include "game_system_logic/system/input_controlled_character_movement.h"
#include "game_system_logic/system/player_character_lock_onto_target.h"
#include "game_system_logic/system/player_character_world_space_input.h"
#include "game_system_logic/system/process_physics_object_lifetime.h"
#include "game_system_logic/system/propagate_changed_transforms.h"
#include "game_system_logic/system/tick_sim_char_mvt_animator.h"
#include "game_system_logic/system/write_entity_transforms_from_physics.h"
#include "game_system_logic/system/write_render_transforms.h"
#include "game_system_logic/world/scene_loader.h"
#include "game_system_logic/world/world_properties.h"
#include "Jolt/Jolt.h"  // @DEBUG
#include "Jolt/Math/Real.h"  // @DEBUG
#include "Jolt/Math/Quat.h"  // @DEBUG
#include "btlogger.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "service_finder/service_finder.h"
#include "settings/settings.h"
#include "timer/timer.h"
#include "timer/watchdog_timer.h"
#include "txp_renderer_public.h"
#include <cstdint>
#include <memory>



#define IMPLEMENT_THIS 0


int32_t main()
{
    BT::initialize_app_settings_from_file_or_fallback_to_defaults();
    auto const& app_settings{ BT::get_app_settings_read_handle() };

    BT::Watchdog_timer main_watchdog;

    // Setup ECS system.
    BT::component::register_all_components();
    BT::Entity_container entity_container;

    // Setup world properties.
    BT::world::World_properties_container world_properties;
    {
        auto& wprops{ world_properties.get_data_handle() };
        wprops.is_simulation_running = false;
    }

    // Setup renderer.
    TXP::Input::Input_handler input_handler;
    TXP::Renderer main_renderer{ entity_container.get_ecs_registry(),
                                 "No Train No Game",
                                 1280,
                                 720,
                                 BTZC_GAME_ENGINE_ASSET_TEXTURE_PATH,
                                 BTZC_GAME_ENGINE_ASSET_SHADER_PATH,
                                 BTZC_GAME_ENGINE_ASSET_MODEL_PATH,
                                 BTZC_GAME_ENGINE_ASSET_ANIM_FRAME_ACTIONS_PATH,
                                 BTZC_GAME_ENGINE_ASSET_ANIMATOR_TEMPLATES_PATH,
                                 [&world_properties](bool flag) {
                                     world_properties.get_data_handle().is_simulation_running =
                                         flag;
                                 },
                                 [&world_properties]() {
                                    return world_properties.get_data_handle().is_simulation_running;
                                 } };

    main_renderer.add_texture("test_ktx_tex", ".ktx2");
    main_renderer.add_material("default_mat", "basic_diffuse", { { "texture0", "test_ktx_tex" } });
    main_renderer.add_material("ProBuilderDefault",
                               "basic_diffuse",
                               { { "texture0", "test_ktx_tex" } });
    main_renderer.add_material("__gradient_mat",
                               "gradient",
                               { { "image", "__hdr_draw_image_color" } });
    main_renderer.add_material_palette("default_material_palette", { "default_mat" });
    main_renderer.add_model("probuilder_example", ".wobj", false, false);
    main_renderer.add_model("simple_combat_char", ".glb", true, true);
    main_renderer.build();

    // Setup physics engine.
    BT::Physics_engine main_physics_engine;

    BT::Raycast_helper::set_physics_engine(main_physics_engine);
#if IMPLEMENT_THIS
    BT::Material_bank::set_camera_read_ifc(&main_renderer);

    // Shaders.
    BT::Shader_bank::emplace_shader(
        "color_unlit",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit.frag"));
    BT::Shader_bank::emplace_shader(
        "color_unlit_lines",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit_lines.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit_lines.frag"));
    BT::Shader_bank::emplace_shader(
        "color_shaded",f
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_shaded.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_shaded.frag"));
    BT::Shader_bank::emplace_shader(
        "textured_shaded",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "textured_shaded.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "textured_shaded.frag"));
    BT::Shader_bank::emplace_shader(
        "post_process",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "post_process.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "post_process.frag"));
    BT::Shader_bank::emplace_shader(
        "skinned_mesh_compute",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "skinned_mesh.comp"));

    // Textures.
    BT::Texture_bank::emplace_texture_2d(
        "default_texture",
        BT::Texture_bank::load_texture_2d_from_file(BTZC_GAME_ENGINE_ASSET_TEXTURE_PATH "grids_1m.jpg",
                                                    3));

    // Materials.
    BT::Material_bank::emplace_material(
        "color_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_shaded(vec3{ 0.5f, 0.225f, 0.3f })));
    BT::Material_bank::emplace_material(
        "textured_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_texture_shaded(BT::Texture_bank::get_texture_2d("default_texture"),
                                                   vec3{ 0.0f, 0.2f, 0.5f },
                                                   vec3{ 0.5f, 0.2f, 0.1f },
                                                   45.0f)));
    BT::Material_bank::emplace_material(
        "debug_physics_wireframe_fore_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.2f, 0.95f, 0.3f },
                                                BT::k_depth_test_mode_front)));
    BT::Material_bank::emplace_material(
        "debug_physics_wireframe_back_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.1f, 0.475f, 0.15f },
                                                BT::k_depth_test_mode_back)));
    BT::Material_bank::emplace_material(
        "debug_selected_wireframe_fore_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.95f, 0.3f, 0.95f },
                                                BT::k_depth_test_mode_front)));
    BT::Material_bank::emplace_material(
        "debug_selected_wireframe_back_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.475f, 0.15f, 0.475f },
                                                BT::k_depth_test_mode_back)));
    BT::Material_bank::emplace_material(  // @INCOMPLETE @TODO
        "debug_lines_fore_material",
        unique_ptr<BT::Material_ifc>(new BT::Material_debug_lines(true)));
    BT::Material_bank::emplace_material(
        "debug_lines_back_material",
        unique_ptr<BT::Material_ifc>(new BT::Material_debug_lines(false)));
    BT::Material_bank::emplace_material(
        "debug_picking_material",
        unique_ptr<BT::Material_ifc>(new BT::Material_debug_picking()));
    BT::Material_bank::emplace_material(
        "post_process",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_impl_post_process(1.0f)));

    // Models.
    // "models": [
    //     "SlimeGirl": {
    //         "type": "obj",
    //         "material": "color_material"
    //     },
    //     ...
    // ]
    BT::Model_bank::emplace_model(
        "unit_box",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "unit_box.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "box_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "box_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "xz_grid",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "stupid_polygon_grid.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "player_model_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "player_model_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "probuilder_example",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "probuilder_example.obj",
                               "textured_material"));
    BT::Model_bank::emplace_model(
        "simple_combat_char",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "simple_combat_char.glb",
                               "textured_material"));
    // BT::Model_bank::emplace_model(  // @NOCHECKIN: REMOVE THIS MODEL ONCE DONE TESTING SKINNING (and remove from `s_scene_as_json`).
    //     "SlimeGirl",
    //     make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "SlimeGirl.glb",
    //                            "textured_material"));
    // BT::Model_bank::emplace_model(  // @NOCHECKIN: REMOVE THIS MODEL ONCE DONE TESTING SKINNING (and remove from `s_scene_as_json`).
    //     "test_gltf",
    //     make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "Leever.glb",
    //                            "textured_material"));

    // Animator templates.
    BT::Animator_template_bank main_anim_template_bank;
#endif // IMPLEMENT_THIS

#if IMPLEMENT_THIS
    // Animation frame action runtime data.
    // "anim_frame_action_runtime_datas": [
    //     "SlimeGirl",
    //     ...
    // ]
    std::vector<std::string> const afa_names{
        "simple_combat_char",
        // "SlimeGirl",
    };
    for (auto const& afa_name : afa_names)
    {
        BT::anim_frame_action::Bank::emplace(
            afa_name + ".btafa",  // @NOTE: Key must match the file name!!!!! This is very very important.  -Thea 2025/08/30
            BT::anim_frame_action::Runtime_data_controls(
                BTZC_GAME_ENGINE_ASSET_ANIM_FRAME_ACTIONS_PATH + afa_name + ".btafa"));
    }
#endif // IMPLEMENT_THIS

    // POPULATE TEST LEVEL (@TODO: Once level loading is implemented, replace this with it)
    // Physics objects.
    // auto player_char_phys_obj{
    //     BT::Physics_object::create_character_controller(main_physics_engine,
    //                                                     true,
    //                                                     0.5f,
    //                                                     2.0f,
    //                                                     1.0f,
    //                                                     { JPH::RVec3(0.0f, 5.1f, 0.0f),
    //                                                       JPH::Quat::sIdentity() }) };
    // player_char_phys_obj->assign_generated_uuid();
    // auto player_char_phys_obj_key =
    //     main_physics_engine.emplace_physics_object(std::move(player_char_phys_obj));

    // auto static_level_terrain_phys_obj{
    //     BT::Physics_object::create_triangle_mesh(main_physics_engine,
    //                                              false,
    //                                              BT::Model_bank::get_model("probuilder_example"),
    //                                              JPH::EMotionType::Static,
    //                                              { JPH::RVec3(0.0f, 1.0f, 0.0f),
    //                                                JPH::Quat::sIdentity() }) };
    // static_level_terrain_phys_obj->assign_generated_uuid();
    // main_physics_engine.emplace_physics_object(std::move(static_level_terrain_phys_obj));

#if IMPLEMENT_THIS
    // Render objects.
    auto& render_object_pool{ main_renderer.get_render_object_pool() };
#endif // IMPLEMENT_THIS

    // BT::Render_object player_char_rend_obj{
    //     BT::Model_bank::get_model("box_0.5_2"),
    //     BT::Render_layer::RENDER_LAYER_DEFAULT,
    //     GLM_MAT4_IDENTITY,
    //     player_char_phys_obj_key };
    // player_char_rend_obj.assign_generated_uuid();
    // auto player_char_rend_obj_key = render_object_pool.emplace(std::move(player_char_rend_obj));

    // BT::Render_object static_level_terrain_rend_obj{
    //     BT::Model_bank::get_model("probuilder_example"),
    //     BT::Render_layer::RENDER_LAYER_DEFAULT,
    //     GLM_MAT4_IDENTITY };
    // static_level_terrain_rend_obj.assign_generated_uuid();
    // render_object_pool.emplace(std::move(static_level_terrain_rend_obj));

#if IMPLEMENT_THIS
    // Hitcapsule solver.
    BT::Hitcapsule_group_overlap_solver hitcapsule_solver;
#endif // IMPLEMENT_THIS

    // Load default scene.
    BT::world::Scene_loader main_scene_loader;
    main_scene_loader.load_scene("_dev_sample_scene.btscene");

    // Setup audio engine.
    BT::audio::initialize();

    // Timer.
    BT::Timer main_timer;
    main_timer.start_timer();

    // Iteration types for main loop.
    enum class Iteration_type
    {
        FIRST_RUNNING_ITERATION,
        RUNNING_ITERATION,
        TEARDOWN_ITERATION,
        EXIT_LOOP,
    };
    BT_TRACE("==== ENTERING MAIN LOOP (FIRST RUNNING ITERATION) ==============");
    Iteration_type iter_type{ Iteration_type::FIRST_RUNNING_ITERATION };

    // Main loop.
    while (iter_type != Iteration_type::EXIT_LOOP)
    {
        BT::logger::notify_start_new_mainloop_iteration();
        main_watchdog.pet();
        main_renderer.poll_input_events();

        float_t delta_time =
            main_physics_engine.limit_delta_time(
                main_timer.calc_delta_time());

        // @NOCHECKIN: @DEBUG: Fun little sfx for audio engine.
        if (iter_type == Iteration_type::FIRST_RUNNING_ITERATION)
        {
            auto snd_key{ BT::audio::mark_snd_required("test_sfx_0.ogg", false, false, false) };
            BT::audio::play_sound(snd_key, BT::audio::volume_to_db(0.25f));
        }

        // Simulation loop.
        main_physics_engine.accumulate_delta_time(delta_time);
        while (main_physics_engine.calc_wants_to_tick() ||  // @TODO: Change the `wants_to_tick()` to something that's not the physics engine. Perhaps a simulation manager or something???  -Thea 2025/10/31
               iter_type == Iteration_type::TEARDOWN_ITERATION)  // Force one iteration if teardown.
        {   // Performance measure.
            BT::Timer perf_timer;
            perf_timer.start_timer();

            // Pre-physics.
            BT::system::process_physics_object_lifetime();

            TXP::Renderer::advance_afa_sim_timer(main_physics_engine.k_simulation_delta_time);
#if IMPLEMENT_THIS
            BT::system::tick_sim_char_mvt_animator();  // Requires animator and AFA.
#endif // IMPLEMENT_THIS

#if IMPLEMENT_THIS
            BT::system::character_broadcast_attack_msg_to_enemies();  // Requires AFA
            BT::system::cpu_character_enemy_detection();  // Requires AFA
#endif // IMPLEMENT_THIS
            BT::system::cpu_character_world_space_input();
            BT::system::player_character_world_space_input();
            BT::system::input_controlled_character_movement();

            // Physics calculations.
            main_physics_engine.update_physics();

            // Post-physics.
            BT::system::write_entity_transforms_from_physics();
            BT::system::propagate_changed_transforms();

            BT::system::player_character_lock_onto_target();

#if IMPLEMENT_THIS
            BT::system::animator_driven_hitcapsule_sets_update();  // Requires AFA
            BT::system::hitcapsule_attack_processing(BT::Physics_engine::k_simulation_delta_time);  // Requires AFA.
#endif // IMPLEMENT_THIS

            // Audio tick.
            BT::audio::update();

            // Performance measure.
            main_renderer.report_performance_time(TXP::PERF_TIME_TYPE_SIMULATION_LOOP,
                                                  perf_timer.calc_delta_time());

            // Only run once if teardown iteration.
            if (iter_type == Iteration_type::TEARDOWN_ITERATION)
                break;
        }

        // Render loop.
        main_physics_engine.calc_interpolation_alpha();
        {   // Performance measure.
            BT::Timer perf_timer;
            perf_timer.start_timer();

            // Run all pre-render systems.
#if IMPLEMENT_THIS
            bool is_afa_editor_context{
                main_renderer_imgui_renderer.is_anim_frame_data_editor_context() };
            if (is_afa_editor_context)
                BT::system::_dev_animation_frame_action_editor();
#endif // IMPLEMENT_THIS

            BT::system::write_render_transforms();
#if IMPLEMENT_THIS
            BT::system::update_selected_entity_debug_render_transform();
#endif // IMPLEMENT_THIS

            if (iter_type < Iteration_type::TEARDOWN_ITERATION)
            {
                main_renderer.set_allow_deformed_render_models(
                    world_properties.get_data_handle().is_simulation_running);

                main_renderer.render_one_frame(delta_time);

#if IMPLEMENT_THIS
                if (is_afa_editor_context)
                    // @HACK: @IMPROVE: Run AFA editor again in case if animator reconfiguration is
                    //   needed from ImGui actions of the render that just happened.
                    // @TODO: Make the func call below a stripped down version that only deals with
                    //        animator reconfiguration, and no processing of the animator, no
                    //        processing of AFA controller/data points.
                    BT::system::_dev_animation_frame_action_editor();
#endif // IMPLEMENT_THIS
            }

            // Performance measure.
            main_renderer.report_performance_time(TXP::PERF_TIME_TYPE_RENDERER_LOOP,
                                                  perf_timer.calc_delta_time());
        }

        // Switch iteration type.
        switch (iter_type)
        {
        case Iteration_type::FIRST_RUNNING_ITERATION:
            // Turn off logging to the console (except for errors and warnings).
            BT_TRACE("Set logger to not print to console (except for errors and warnings).");
            BT::logger::set_logging_print_mask(  // @TODO: @FIXME: Make bitmask support better. This sucks ass.  -Thea 2025/11/23
                (BT::logger::Log_type)((uint32_t)BT::logger::ERROR | (uint32_t)BT::logger::WARN));

            BT_TRACE("==== ENTERING RUNNING ==========================================");
            iter_type = Iteration_type::RUNNING_ITERATION;
            break;

        case Iteration_type::RUNNING_ITERATION:
            if (main_renderer.is_requesting_shutdown())
            {   // Enter teardown.
                main_scene_loader.unload_all_scenes();

                BT::logger::set_logging_print_mask(BT::logger::ALL);

                BT_TRACE("==== ENTERING TEARDOWN =========================================");
                iter_type = Iteration_type::TEARDOWN_ITERATION;
            }
            break;

        case Iteration_type::TEARDOWN_ITERATION:
            BT_TRACE("==== EXITING MAIN LOOP =========================================");
            iter_type = Iteration_type::EXIT_LOOP;
            break;

        case Iteration_type::EXIT_LOOP:
            // How did you get here? The loop should've exited.
            assert(false);
            break;
        }

        // Tick scene loader.
        main_scene_loader.process_scene_loading_requests();
    }

    // Write final state of settings file.
#if IMPLEMENT_THIS
    main_renderer.save_state_to_app_settings();
#endif // IMPLEMENT_THIS
    BT::save_app_settings_to_disk();

    // Show stats prior to cleanup.
    BT_TRACEF("Post-teardown statistics:\n"
              "  Num scenes                        : %i\n"
              "  Num entities                      : %i\n"
              "  Num ECS entities                  : %i\n"
              "  Num physics objects               : %i\n"
#if IMPLEMENT_THIS
              "  Num render objects                : %i\n"
              "  Num hitcapsule grp sets in solver : %i\n"
#endif // IMPLEMENT_THIS
              ,
              main_scene_loader.get_num_loaded_scenes(),
              entity_container.get_num_entities(),
              entity_container.get_ecs_registry().view<entt::entity>().size(),
              main_physics_engine.get_num_physics_objects()
#if IMPLEMENT_THIS
              ,
              main_renderer.get_render_object_pool().get_num_render_objects(),
              hitcapsule_solver.get_num_group_sets()
#endif // IMPLEMENT_THIS
    );

    return 0;
}
