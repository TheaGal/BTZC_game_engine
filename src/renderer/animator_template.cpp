#include "animator_template.h"

#include "../btzc_game_engine.h"
#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"
#include "animator_template_types.h"
#include "btjson.h"

#include <cassert>
#include <sstream>
#include <iterator>
#include <string>


BT::Animator_template_bank::Animator_template_bank()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Animator_template_bank, this);
}

BT::Animator_template const& BT::Animator_template_bank::load_animator_template(
    std::string const& anim_template_name)
{
    if (m_anim_template_cache.find(anim_template_name) == m_anim_template_cache.end())
    {   // Load from disk.
        json root = json_load_from_disk(BTZC_GAME_ENGINE_ASSET_ANIMATOR_TEMPLATES_PATH +
                                        anim_template_name);

        // Fill in new struct.
        Animator_template new_template = root;

        // Cook animator template variables.
        new_template.variables_cooked.resize(new_template.variables.size());
        for (size_t i = 0; i < new_template.variables.size(); i++)
        {   // Get tokens of variable string.
            std::vector<std::string> tokens;
            {
                std::istringstream iss(new_template.variables[i]);
                std::copy(std::istream_iterator<std::string>(iss),
                          std::istream_iterator<std::string>(),
                          std::back_inserter(tokens));
                assert(tokens.size() == 2);
            }

            // Start cookin'
            auto& ckd{ new_template.variables_cooked[i] };

            // Convert type.
            if      (tokens[0] == "bool")  ckd.type = anim_tmpl_types::Animator_variable::TYPE_BOOL;
            else if (tokens[0] == "int")   ckd.type = anim_tmpl_types::Animator_variable::TYPE_INT;
            else if (tokens[0] == "float") ckd.type = anim_tmpl_types::Animator_variable::TYPE_FLOAT;
            else if (tokens[0] == "trig")  ckd.type = anim_tmpl_types::Animator_variable::TYPE_TRIGGER;
            else assert(false);

            // Convert name.
            ckd.var_name = tokens[1];
        }

        // Cook animator template state transitions.
        for (auto& state_trans : new_template.state_transitions)
        {
            auto& ckd{ state_trans.cooked };

            // Convert from-to-state.
            static auto const s_find_anim_state_idx =
                [](std::vector<Animator_template::Animator_state> const& anim_states,
                   std::string const& anim_state_name) {
                    for (size_t j = 0; j < anim_states.size(); j++)
                        if (anim_state_name == anim_states[j].state_name)
                        {
                            return j;
                        }

                    BT_ERRORF("Failed: Could not find anim state name: %s",
                              anim_state_name.c_str());
                    assert(false);
                    return (size_t)-1;
                };

            ckd.from_to_state.first.resize(state_trans.from_to_state.first.size());
            for (size_t i = 0; i < state_trans.from_to_state.first.size(); i++)
                ckd.from_to_state.first[i] =
                    s_find_anim_state_idx(new_template.animator_states,
                                          state_trans.from_to_state.first[i]);

            ckd.from_to_state.second = s_find_anim_state_idx(new_template.animator_states,
                                                             state_trans.from_to_state.second);

            // Get tokens of condition string.
            std::vector<std::string> tokens;
            {
                std::istringstream iss(state_trans.condition);
                std::copy(std::istream_iterator<std::string>(iss),
                          std::istream_iterator<std::string>(),
                          std::back_inserter(tokens));
            }

            size_t tidx{ 0 };
            bool expecting_and_token{ false };
            while (tidx < tokens.size())
            {   // Get subset of tokens.
                std::vector<std::string> sub_tokens;
                sub_tokens.reserve(3);
                for (size_t sti = 0; sti < 3 && tidx + sti < tokens.size(); sti++)
                {   // Get next 3 tokens into sub-tokens, if available.
                    sub_tokens.emplace_back(tokens[tidx + sti]);
                }
                assert(sub_tokens.size() == 1 || sub_tokens.size() == 3);

                // Check for special case AND token.
                if (expecting_and_token)
                {
                    if (sub_tokens[0] != "and")
                    {
                        assert(false);
                    }
                    expecting_and_token = false;
                    tidx += 1;
                    continue;
                }

                // New condition.
                anim_tmpl_types::Animator_state_transition::Condition ncd;

                // Convert var idx.
                auto var_type{ anim_tmpl_types::Animator_variable::TYPE_INVALID };

                if (sub_tokens[0] == "ON_ANIM_END")
                {   // Special case.
                    ncd.condition_var_idx = anim_tmpl_types::k_on_anim_end_var_idx;
                    var_type = anim_tmpl_types::Animator_variable::TYPE_TRIGGER;
                }
                else
                {
                    bool found{ false };
                    for (size_t i = 0; i < new_template.variables_cooked.size(); i++)
                        if (sub_tokens[0] == new_template.variables_cooked[i].var_name)
                        {   // Found var name!
                            ncd.condition_var_idx = i;
                            var_type = new_template.variables_cooked[i].type;
                            found = true;
                            break;
                        }

                    if (!found)
                    {
                        BT_ERRORF("Var name not found: %s", sub_tokens[0].c_str());
                        assert(false);
                    }
                }

                // Convert compare op and value.
                switch (var_type)
                {
                case anim_tmpl_types::Animator_variable::TYPE_BOOL:
                    if      (sub_tokens[1] == "eq")  ncd.compare_operator = ncd.COMP_EQ;
                    else if (sub_tokens[1] == "neq") ncd.compare_operator = ncd.COMP_NEQ;
                    else assert(false);

                    if      (sub_tokens[2] == "false") ncd.compare_value = anim_tmpl_types::k_bool_false;
                    else if (sub_tokens[2] == "true")  ncd.compare_value = anim_tmpl_types::k_bool_true;
                    else assert(false);
                    break;

                case anim_tmpl_types::Animator_variable::TYPE_INT:
                    if      (sub_tokens[1] == "eq")      ncd.compare_operator = ncd.COMP_EQ;
                    else if (sub_tokens[1] == "neq")     ncd.compare_operator = ncd.COMP_NEQ;
                    else if (sub_tokens[1] == "less")    ncd.compare_operator = ncd.COMP_LESS;
                    else if (sub_tokens[1] == "leq")     ncd.compare_operator = ncd.COMP_LEQ;
                    else if (sub_tokens[1] == "greater") ncd.compare_operator = ncd.COMP_GREATER;
                    else if (sub_tokens[1] == "geq")     ncd.compare_operator = ncd.COMP_GEQ;
                    else assert(false);

                    assert(false);  // @TODO: Implement str-to-int here!
                    break;

                case anim_tmpl_types::Animator_variable::TYPE_FLOAT:
                    if      (sub_tokens[1] == "eq")      ncd.compare_operator = ncd.COMP_EQ;
                    else if (sub_tokens[1] == "neq")     ncd.compare_operator = ncd.COMP_NEQ;
                    else if (sub_tokens[1] == "less")    ncd.compare_operator = ncd.COMP_LESS;
                    else if (sub_tokens[1] == "leq")     ncd.compare_operator = ncd.COMP_LEQ;
                    else if (sub_tokens[1] == "greater") ncd.compare_operator = ncd.COMP_GREATER;
                    else if (sub_tokens[1] == "geq")     ncd.compare_operator = ncd.COMP_GEQ;
                    else assert(false);

                    assert(false);  // @TODO: Implement str-to-float here!
                    break;

                case anim_tmpl_types::Animator_variable::TYPE_TRIGGER:
                    ncd.compare_operator = ncd.COMP_EQ;
                    ncd.compare_value = anim_tmpl_types::k_trig_triggered;
                    break;

                default: assert(false); break;
                }

                // Insert in new condition.
                ckd.list_of_and_conditions.emplace_back(std::move(ncd));
                expecting_and_token = true;
                tidx += (var_type == anim_tmpl_types::Animator_variable::TYPE_TRIGGER ? 1 : 3);
            }
        }

        m_anim_template_cache.emplace(anim_template_name, std::move(new_template));
    }


    // Grab template from cache.
    return m_anim_template_cache.at(anim_template_name);
}

void BT::Animator_template_bank::load_animator_template_into_animator(
    Model_animator& animator,
    std::string const& anim_template_name)
{
    auto const& anim_temp{ load_animator_template(anim_template_name) };

    // Write to model animator.
    std::vector<anim_tmpl_types::Animator_state> anim_states;
    anim_states.reserve(anim_temp.animator_states.size());
    for (auto const& temp_anim_state : anim_temp.animator_states)
    {
        auto state_type{ temp_anim_state.state_type == "single_anim"
                             ? anim_tmpl_types::Animator_state::SINGLE_ANIM
                             : anim_tmpl_types::Animator_state::BLENDTREE };

        std::vector<anim_tmpl_types::Animator_state::Blend_anim> blend_anims;
        if (state_type == anim_tmpl_types::Animator_state::BLENDTREE)
        {
            for (auto const& blend_anim : temp_anim_state.blend_anims)
                blend_anims.emplace_back(animator.get_model_animation_idx(blend_anim.anim_name),
                                         blend_anim.value);
        }

        anim_states.emplace_back(temp_anim_state.state_name,
                                 state_type,
                                 state_type == anim_tmpl_types::Animator_state::SINGLE_ANIM
                                     ? animator.get_model_animation_idx(temp_anim_state.anim_name)
                                     : (uint32_t)-1,
                                 temp_anim_state.blend_var,
                                 std::move(blend_anims),
                                 temp_anim_state.speed,
                                 temp_anim_state.loop);
    }

    // @TODO: Also include transition states in model animator.

    std::vector<anim_tmpl_types::Animator_state_transition> anim_state_transitions;
    anim_state_transitions.reserve(anim_temp.state_transitions.size());
    for (auto const& temp_state_trans : anim_temp.state_transitions)
    {
        anim_state_transitions.emplace_back(temp_state_trans.cooked);
    }

    animator.configure_animator_states(anim_states,
                                       anim_temp.variables_cooked,
                                       anim_state_transitions);
}
