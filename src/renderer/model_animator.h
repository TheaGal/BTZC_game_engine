#pragma once

#include "../animation_frame_action_tool/runtime_data.h"
#include "animator_template_types.h"
#include "btglm.h"
#include "uuid/uuid.h"

#include <atomic>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>


namespace BT
{

struct Model_joint;

struct Model_skin
{
    mat4 baseline_transform = GLM_MAT4_IDENTITY_INIT;
    mat4 inverse_global_transform = GLM_MAT4_IDENTITY_INIT;
    std::unordered_map<std::string, uint32_t> joint_name_to_idx;
    std::vector<Model_joint> joints_sorted_breadth_first;
};

struct Model_joint
{
    std::string name;
    mat4 inverse_bind_matrix;
    uint32_t parent_idx{ (uint32_t)-1 };  // @NOTE: Idx instead of pointer for cache lookup.  -Thea 2025/07/10
    std::vector<Model_joint*> children;
};

struct Model_joint_animation_frame
{
    struct Joint_local_transform
    {
        vec3 position;
        versor rotation;
        vec3 scale;

#if 0  /* @NOTE: I really don't think I can handle step interpolation. It's too 細かい from gltf to use here. */
        // (FOR NOW OR MAYBE FOREVER) interpolation type is ignored and only linear is used
        // at least for skeletal animation.
        enum Interpolation_type
        {
            INTERP_TYPE_LINEAR = 0,
            INTERP_TYPE_STEP,
            NUM_INTERP_TYPES
        } interp_type;
#endif  // 0

        Joint_local_transform interpolate_fast(Joint_local_transform const& other,
                                               float_t t) const;
    };
    std::vector<Joint_local_transform> joint_transforms_in_order;

    vec3 root_motion_delta_pos;
};

class Model_joint_animation
{
public:
    Model_joint_animation(Model_skin const& skin,
                          std::string name,
                          std::vector<Model_joint_animation_frame>&& animation_frames);

    std::string get_name() const { return m_name; }
    size_t get_num_frames() const { return m_frames.size(); }

    enum Rounding_func{ FLOOR, CEIL };
    uint32_t calc_frame_idx(float_t time, bool loop, Rounding_func rounding) const;

    using Joint_local_transform_set_t =
        std::vector<Model_joint_animation_frame::Joint_local_transform>;

    Joint_local_transform_set_t calc_joint_local_transforms_interpolated(
        float_t time,
        bool loop,
        bool root_motion_zeroing) const;
    Joint_local_transform_set_t calc_joint_local_transforms_floored(float_t time,
                                                                    bool loop,
                                                                    bool root_motion_zeroing) const;

    static Joint_local_transform_set_t blend_joint_local_transform_sets(
        Joint_local_transform_set_t const& a,
        Joint_local_transform_set_t const& b,
        float_t blend_t);

    // @TODO: @THEA: This func should get moved to `Model_animator` instead!!!!
    // @NOCHECKIN: Do ^^ above ^^
    void calc_joint_matrices(Joint_local_transform_set_t const& joint_local_transforms,
                             std::vector<mat4s>& out_joint_matrices) const;

    void get_root_motion_delta_pos_at_frame(uint32_t frame_idx,
                                            vec3& out_root_motion_delta_pos) const;

    static constexpr float_t k_frames_per_second{ 60.0f };

private:
    Model_skin const& m_model_skin;

    std::string m_name;
    std::vector<Model_joint_animation_frame> m_frames;
};

class Model;

class Model_animator
{
public:
    Model_animator(Model const& model, bool use_root_motion);

    Model_skin const& get_model_skin() const;

    void configure_animator_states(
        std::vector<anim_tmpl_types::Animator_state> animator_states,
        std::vector<anim_tmpl_types::Animator_variable> animator_variables);

    /// Information to create a jump queue.
    struct Jump_queue_create
    {
        std::string queue_name;
        bool default_is_watching;
    };

    void configure_anim_frame_action_controls(
        std::vector<Jump_queue_create>&& jump_queues,
        anim_frame_action::Runtime_data_controls const* anim_frame_action_controls,
        UUID resp_entity_uuid);

    std::vector<anim_tmpl_types::Animator_state> const& get_animator_states() const;
    anim_tmpl_types::Animator_state const& get_animator_state(size_t idx) const;
    anim_tmpl_types::Animator_state& get_animator_state_write_handle(size_t idx);

    size_t get_num_animator_variables() const;
    anim_tmpl_types::Animator_variable const& get_animator_variable(size_t idx) const;
    anim_tmpl_types::Animator_variable& get_animator_variable_write_handle(size_t idx);

    /// State set. Once an animation state finishes, the animator changes to the next state in the
    /// `anim_state_indices` list. Once the final state finishes, it will either stop, or loop
    /// depending on `loop_final_state`.
    struct Animator_state_set
    {
        std::vector<uint32_t> anim_state_indices;
        bool loop_final_state;
    };

    /// Changes state-set.
    void change_state_set(Animator_state_set const& to_state_set);

    size_t get_model_animation_idx(std::string anim_name) const;
    Model_joint_animation const& get_model_animation(size_t idx) const;

    /// Sets a variable inside the state machine.
    void set_bool_variable(std::string const& var_name, bool value);

    /// Sets a variable inside the state machine.
    void set_int_variable(std::string const& var_name, int32_t value);

    /// Sets a variable inside the state machine.
    void set_float_variable(std::string const& var_name, float_t value);

    /// Gets a variable inside the state machine.
    float_t get_float_variable(std::string const& var_name) const;

    /// Sets a variable inside the state machine.
    void set_trigger_variable(std::string const& var_name);

    /// Sets time for all timer profiles of the animator.
    void set_time(float_t time);

    /// Profile enum for which timing of the animator to base calculations off of.
    enum Animator_timer_profile
    {
        SIMULATION_PROFILE,
        RENDERER_PROFILE,
    };

    /// Updates the animator, supplying a deltatime.
    /// There are two animator timers, so you need to give which timer to update.
    void update(Animator_timer_profile profile, float_t delta_time);

    /// Calculates the set of joint matrices, interpolated.
    /// Also allows for root motion zeroing.
    void calc_anim_pose(Animator_timer_profile profile,
                        bool root_motion_zeroing,
                        std::vector<mat4s>& out_joint_matrices) const;

    /// Gets whether root motion is enabled or not on this animator.
    bool get_is_using_root_motion() const;

    /// Calculates the set of joint matrices, floored. Note this one will be faster.
    /// Also allows for root motion zeroing.
    void get_anim_floored_frame_pose(Animator_timer_profile profile,
                                     bool root_motion_zeroing,
                                     std::vector<mat4s>& out_joint_matrices) const;

    /// Gets the root motion delta pos of the current frame.
    void get_anim_root_motion_delta_pos(Animator_timer_profile profile,
                                        vec3& out_root_motion_delta_pos) const;

    /// Gets reference to AFA (animation frame action) data.
    anim_frame_action::Runtime_controllable_data& get_anim_frame_action_data_handle();

    /// Documentation type for a control command.
    struct Ctrl_cmd_documentation
    {
        struct Name_w_desc
        {
            std::string name;
            std::string desc;
        } cmd;

        struct Name_w_desc_w_type
        {
            std::string name;
            std::string desc;
            std::string type;
        };
        std::vector<Name_w_desc_w_type> argv;

        std::function<void(Model_animator&, uint32_t, bool, bool, std::vector<std::string> const&)>
            exec_fn;
    };

    /// Gets documentation for all control cmds.
    static std::vector<Ctrl_cmd_documentation> const& get_control_command_codes_documentation();

    /// Adds a state set to a jump queue.
    void emplace_jump_queue_state_set(std::string const& jump_queue_name,
                                      Animator_state_set const& state_set,
                                      float_t queue_expire_time);

    /// Resets jump queue watchlist to default values.
    void reset_jump_queue_watchlist();

    /// Sets whether watching a jump queue.
    void set_watch_jump_queue(std::string const& jump_queue_name, bool watch);

    /// Fetches/pops first top priority state-set from set of watching jump queues.
    Animator_state_set const* pop_one_state_set();

private:
    std::vector<Model_joint_animation> const& m_model_animations;
    Model_skin const& m_model_skin;

    // @TEMP: Super simple animator right here for now.
    std::atomic_uint32_t m_current_state_idx{ 0 };

    // @NOTE: Times need to be atomic since `change_state_idx()` and `set_time()` can be called from
    //        any thread.
    using animator_time_t = typename std::atomic<float_t>;
    using animator_frame_t = typename std::atomic_uint32_t;

    animator_time_t& get_profile_time_handle(Animator_timer_profile profile) const;

    animator_time_t m_sim_time{ -1.0f };  // -1 for showing timer is unset on first update().
    animator_time_t m_rend_time{ -1.0f };

    animator_frame_t& get_profile_prev_frame_handle(Animator_timer_profile profile) const;

    animator_frame_t m_sim_prev_frame{ (uint32_t)-1 };  // -1 means unset.

    bool m_is_using_root_motion;

    /// Type for interpreted code.
    using cmd_code_t = anim_frame_action::Runtime_data_controls::Data::
        Animation_frame_action_timeline::Region::Control_command;

    /// Interprets and executes sent command code.
    void execute_command_code(cmd_code_t const& cmd_code,
                              uint32_t row_idx,
                              bool is_reg_first_frame,
                              bool is_reg_last_frame);

    ///////////////////////////////////////////////////

    std::vector<anim_tmpl_types::Animator_state> m_animator_states;
    std::vector<anim_tmpl_types::Animator_variable> m_animator_variables;

    struct Jump_queue_data
    {
        bool is_watching;
        bool default_is_watching;
        std::vector<Animator_state_set const*> state_set_queue;
    };
    std::unordered_map<std::string, Jump_queue_data> m_jump_queue_name_to_jump_queue_map;

    anim_frame_action::Runtime_data_controls const* m_anim_frame_action_controls{ nullptr };
    anim_frame_action::Runtime_controllable_data m_anim_frame_action_data;

    anim_tmpl_types::Animator_variable& find_animator_variable(std::string const& var_name);
    anim_tmpl_types::Animator_variable const& find_animator_variable_const(std::string const& var_name) const;

    struct Blend_value_result
    {
        uint32_t anim_idx_a;
        uint32_t anim_idx_b;
        float_t blend_t;
    };
    Blend_value_result calc_blend_value_ffffffff(
        anim_tmpl_types::Animator_state const& anim_state) const;
};

}  // namespace BT
