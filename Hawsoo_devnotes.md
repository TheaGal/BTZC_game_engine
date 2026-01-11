# Hawsoo's Dev notes.

## Console window implementation.

- There needs to be like a way to retrieve all the data from the logger. Or maybe there could just be a `get_head_and_tail()` func with then just scanning the applicable rows.
- Oh and then a clear logs func.
- For disabling the logger, a message could just get sent "Switching to in-editor console logger" and then `set_logging_print_mask(NONE)` could get called, and then undone when the window is gone.

- Aaaaggghhh it looks like there's not gonna be anymore ansi coloring in the console bc it's kinda haaaarrd ^_^;

- Ok this seems done for now.

- NEVERMIND!!! The logs printed are off.
    - [x] Fixed.


## Animation authoring tool window.

- Gonna use blender-style context switching like they got on the top bar.
    - Well, except it will be in the main menubar as like a "Context: Level Editor"

> I just learned about `ImGui::PushItemWidth(ImGui::GetFontSize() * -10)`. Put it after beginning a new window and it will align all of the widgets to a constant width instead of making them take up 2/3rds of the window width which was hecka annoying.

- So on a bit of a UI adventure tangent, but I decided there will be the ability to have multiple levels in the hierarchy. Each level will be represented by a root object in the hiearachy with an empty gameobject any kind of transform (ideally just position is changed here).
    - Or maybe it could just be a dropdown like in Unity???
    - Or maybe we could just ignore it all and just change the UI to have one level loaded.
        - Like grayed out or something.

- [x] Get simple context switching.

- Dang, like ImGui code just is monstrous how large it gets. Crazy.

- [x] Make window IDs separate with context switching.

- [x] Add placeholder for:
    - Timeline thingy.
    - Model selector.

- I'm starting to use the living room compy which it would be nice to have an imgui zoom thing.
    - [x] Have a `direct_lookup_table` for the input handler.
    - [x] Implement zoom with ctrl+(-/=)

- These are the things I want the anim authoring tool to be able to handle:
    - Inside the timeline:
        - Temporary state variable overrides.
            - `in_deflect_window : bool`
            - `lerp_to(x) : float` <- @NOTE: Happens over the course of the bar.
        - Events.
            - `play_sfx_fart()` <- @NOTE: Only happens on rising edge of event.
        - Inserting hurtboxes.
            - `use_hurtbox(x)` <- @NOTE: While in this bar, displays previously authored hurtbox.
    - Inside the hit/hurtbox creator:
        - Hitbox/Hurtbox create capsules, add tags, and assign to 1-2 bones.
            - Tags are used for enabling/disabling during timeline.
            - Hitboxes are default on, and hurtboxes are default off.

> Yknow, it really just feels like the best kind of developing is when I play it like Sudoku. Just look at stuff I wanna change and go and do it. Instead of getting overwhelmed, pull back my pace and just take it in a relaxed, slow way.

- I'm just vibe coding rn. Setting in different things and stuff.

- I need to change the region creation to change to non-clicking (so Shift+a like blender it is).

> It really seems to be that game dev and software dev for this matter should be very therapeutic. An experience where one simply sits down and starts vibing w the code. I am just having a blast w the time I spend just sitting and thinking. just sitting and working wo any convos or distractions. I think in the past month I’ve learned so much about how I can be effective thru pacing myself w running and being effective there. its been super fun.

- for this upcoming part it seems like getting the model selector working and connected to some model rendering in the main view is needed. perhaps have a render object that you use? or maybe there needs to be some kind of if statement to switch between level editor/gameplay and the animation editor.

> some thoughts  about the player and animation state machine. it seems like there should be a small set of states (eg: idle, run, jump, knockback, dodge, guard, attack, sheath, unsheath) but these can have many different variations, and before entering the state a variation key is inserted. for example, having "bool weapon_drawn" or "int variation" and this could change the type of attack, breathing style for unsheathing, and guarding (ie midair guarding, sprinting and guarding to a stop) and stuff. then this info could get ingested by the game to generate anim strings. this perhaps may make a lot of work but this should simplify the state machine a lot!!
- ^^^REMEMBER FOR @FUTURE^^^

- Ok so for this next part I need to create a specific script that accesses a global block of data for the animator editor stuff.
    - First thing we'll do is get the animated model to switch depending on the selected model and then select the playing animation and the position in the animation to play depending on the controls.

- Yoooo the tool kinda feels complete as soon as the animations just played with the timeline. Hahahaha feels kinda unreal.

- I think the next thing to work on here is getting the event system all put together and saved into a json file.
    - There also needs to be a main struct to work with that will go well with the animator and the editor.
    - And then a serialization-ifc attached to this struct I think.

- In the `.btafa` file, I ended up putting the key "animated_model_name" which connected to the animated model. I think that this one-to-one connection might not have been the best tho. Idk... I'll have to refactor in the future if needed.

> Mmmmm for some reason I keep on feeling like I'm done with this tool, but I can't really use it for anything lol.

> So it has the ability to create and edit regions and ctrl items, I think that now I just need to match the control items to what they're actually mutating.

- Here's what I want mutated.
    > @NOTE: The below are parts of the runtime data. It should have a copy made and the animator writes to this. Write values if `w` or override values if `o`.
    - Floats
        - Set like this: `float_name:w:3.14444` (On the rising edge of the region, the float `float_name` is set to 3.14444 and writes the value (`w`) persistently.)
        - Interp persistently like this: `float_name:w:0.0->3.5` (For the duration of the region, the float `float_name` lerps from 0.0 to 3.5, and it writes the value (`w`) persistently.)
    - Bools
        - Override like this: `bool_name:o:true` (For the duration of the region, the bool `bool_name` is set to true, but only overwritten, not persistent (`o`).)
    - Rising-edge events
        - Show

    - Float names
        - model_opacity: range(0-1), default(1.0)
        - turn_speed: range(>=0), default(0.0)
        - move_speed: range(∞), default(0.0)  <<@NOTE: `move_speed` will cause character to start moving along the facing direction. Knockback is a negative number since this is moving back.
        - gravity_magnitude: range(∞), default(1.0)  <<@NOTE: Gravity is (0.0, -98.0, 0.0), but this value gets multiplied by gravity to get actual gravity applied to this character.
        -

    - Bool names
        - is_parry_active: default(false)
        - can_move_exit: default(true)
        - can_guard_exit: default(true)
        - can_attack_exit: default(true)
        - blade_has_mizunokata: default(false)
        - blade_has_honoonokata: default(false)
        - show_hurtbox_bicep_r: default(false)
        - hide_hitbox_leg_l: default(false)

    - Rising-edge event names
        - play_sfx_footstep
        - play_sfx_ready_guard
        - play_sfx_blade_swing
        <!-- - play_sfx_blade_hit_flesh -->  <<@NOTE: These are going to be called from world interaction, not animation.
        <!-- - play_sfx_blade_hit_stone -->
        - play_sfx_hurt_vocalize_human_male_mc
        - play_sfx_guard_receive_hit
        - play_sfx_deflect_receive_hit
        -

- Okay, so it seems like there's a pretty good basis for how the timeline should work.

- [x] FIX: Saving the timeline looks like there's not a proper reset for the model animator after, so fix that.
- [x] FIX: Upon renaming ctrl item labels, reload what they're supposed to be doing (same func as when they're loaded in `serialize()`)
    - The program should crash if the label is not correct upon renaming
    - [x] And creating a new ctrl item.
        - It seems like the data type and stuff is just uninitialized for the creation case so it fails in `get_data_type()`'s unknown switch branch.


## Make custom hit/hurt capsule physics implementation.

- Mmmm I'm doing all this work to make it so that the hitcapsule script can add hitcapsules to a game object.
    - But it feels like this stuff should be controlled by the btafa file, not this.
    - Maybe add ability to create scripts dynamically?
    - Then add btafa file can create these groups dynamically.  <---- This.

```cpp
auto my_transform{ Transform::identity() };
auto my_model{ Model_loader::load_model("SlimeGirl.glb") };
auto my_animator{
    Animator_factory::create_animator_from_template(my_model,
                                                    "SlimeGirl.btanitor") };
auto my_anim_frame_action{
    Anim_frame_action_loader::load_anim_frame_action(my_model,
                                                     my_animator,
                                                     "SlimeGirl.btafa") };
Render_object my_render_obj{ my_transform,
                             Render_layer::VISIBLE,
                             my_model,
                             my_animator };

while (running_game_loop)
{
    // ...

    // Runs at same rate as physics.
    // Updates hitcapsules, events, and variables for simulation that are driven
    // by animation.
    my_anim_frame_action.update();

    // ...
}
```


- [x] Rewrite the `script__dev_anim_editor_tool_state_agent.cpp` to use animator states and templates instead of anim indices.
- [x] I think ImGui for the editor also needs to get reworked bc it's probably using the animator indices too.

- [x] Reincorporate hitcapsule groups.
- [x] Load extra part in btafa and add hitcapsule groups.
    - I think that there needs to be a way to prevent self-collision, so grouping hitcapsule groups together into an entity-owning set of groups would be good.
    - [x] initial loading
    - [x] `deep_clone()`
        - Tried to copy the hitcapsule groups but then copy constructors are deleted.
        - Just defined the copy ctor.
        - Ok, just essentially turned off the `Type const m_type;` in order to reinstate the implicit copy ctor stuff.
    - [x] Use `script_hitcapsule` to get a handle into the bool data handle, and then poll whether the hitcapsules are visible or not based off the labels.
        - Ok I'm doing it a bit different, since I'm deferring the functionality to the animator itself, calling it from the script.
        - [x] Create add/remove script dynamically interface.
        - [x] Use dev anim editor script to dynamically add the script.
            - [x] initial
            - [x] Defer actually mutating the scripts until after the scripts are done running.
                - [x] Adds call.
                - [x] Writes deferring funcs.
        - [x] Write funcs for updating hitcapsule `.enabled` flags
        - [x] Write func for updating hitcapsule transforms.
            - [x] `update_hitcapsule_transform_to_joint_mats()`.
            - [x] Write func `get_joint_matrices_at_frame()`.
    - Okay, at this point everything should be working!!! Now we just need to see it to believe it!

- [x] Add visual representation of when hitcapsules are active.
    - Use `debug_render_job.h` for now.
    - [x] Write funcs to submit the capsules to debug render pool.
    - [x] Write `emplace_debug_line_based_capsule()` with regular `emplace_debug_line()`.
        - [x] Initial
        - It works when I enable/disable on the timeline!!!
        - [x] BUGFIX: Saving timeline doesn't work again...
            - [x] Seems like it works now that the `k_num_lines` is increased? (1024 -> 16384)
                - [x] Added an assert to make sure that the num active line draw jobs (`m_active_indices`) does not exceed the max `k_num_lines`.
            - [x] Maybe anims not switching when the state changes in the gui is a related issue??
                - BUT FIX THIS IMMEDIATELYYYY
        - [x] Shorten the drawing timeout so that it doesn't have a bunch of overlapping draws.
    - [x] Connect a capsule to bone of model to test now.

- [x] Create hitcapsule editor (and group assigner) ImGui.
    > Honestly, just making it so that it edits the positions and radius of the capsules would be fine, instead of having to worry about creating new capsules (bc that all can be done by editing the txt in `.btafa` files).
- [x] It kinda seems like more stuff needs to get checked.
    - Multiply `origin_a` and `origin_b` by the bone matrix and then display those as well (or, just maybe make these editable fields inside imgui too (or just disabled-editable (so... just viewable))).
        - [x] misc, have `calcd_origin_a/b` also get mutated when changing the `origin_a/b` in the imgui editor for capsules that don't recalc their `calcd_origin_a/b` after initialization.
    - Ok after positioning in the right spot w `0_TPose`, it works perfectly.

- @THOUGHT: I realllllly reaaallllyyyy wish that there was a multi-view editor (1 reg view, 3 ortho axis views) and that would make this a lot easier to work with.

- [x] Allow saving to `.btafa` from gui. Save changes if dirty.
    - [x] Make dirty more strict, where you can't change the anim state while it's dirty
        - [x] Tooltip for that too.

- [ ] ~~Write and use batched version of `emplace_debug_line()` for `emplace_debug_line_based_capsule()` inner func.~~
    - Do this in the future.

- [x] Fix reloading back into imgui anim frame data editor context
    - It's the issue w using static vars everywhere I think.

- [x] Add the hitcapsule group sets into game mode.
    - [ ] ~~Assign `model_animator`.`m_anim_frame_action_controls` with valid controls.~~
    - [ ] ~~Assign `model_animator`.`m_anim_frame_action_data`.`hitcapsule_group_set` with hitcapsule group set.~~
    - [x] (@NOTE: These two vars^^ are taken care of with this func call) Configure animator with `configure_anim_frame_action_controls()`
    - [ ] ~~Have multiple sets interact with each other.~~
        - Do this in the future!

- [x] BUGFIX: For some reason processing script list mutation requests crashes.
    - Answer: capturing `this` in the static lambda actually breaks if something else wants to be used. So I just removed the lambda.

> @FUTURE: Pull "hitcapsule_group_set" out of .btafa file.


## Misc things I want please please please.

- [x] Make simple viewport changing buttons for +-X, +-Y, +-Z positions.
    - This is for making editing the hitcylinders easier.
    - [x] Initial
    - [x] Zoom and move.
        - Right click to click
        - [x] Basic works.
        - [x] Calc basis vectors correctly.

- [ ] ~~Remove "change viewport with f1" button from main viewport.~~


## Use simple anim dude and make dummy character and create fighting animation movesets.

> Cece suggested naming the dummy character "Frank-e", so I'll name them that.

- [x] Make simple sample anim.
- [x] Create capsules and integrate into the btafa and btanitor systems.

- [x] BUGFIX root bone motion disappeared for some reason.
    - Ummm, ig it was a blender bug? Bc closing and reopening just fixed it.

> I realize that I think the simulation thread of the game needs to hold the master timing. I think that rendering should be its own thing and then the simulation needs to be the sole proprietor of everything. I think having animators move extremely consistently (i.e. on an integer frame tick instead of float increments), ESPECIALLY the ones that have hitcapsules attached to them, is necessary for good swordplay.
>
> So basically, the game runs unhindered on the main thread for simulation. Once two simulations are done running, then a perpetual render job is started, where it has its own `deltatime`, thread, etc. It is started at the beginning of the first simulation (time 0.0), and it should theoretically not catch up to the simulation thread. If that happens, then the render thread should clamp its global `time` to right before the frame it's waiting on.
>
> But then, the issue comes: what hz should this run at? 30? 40? 50? 60? It needs to be a set number to be consistent. To high and it'll be harder to run too.

- ^^The above^^, I don't think I'll worry about it right now. Since there are things like mouse camera controls, mouse clicking and typing (i guess just UI interation stuff?) that I don't want to run inside the simulation thread, I'll need to think about this some more.
    - I think the main point I was trying to make above is that having the hitcapsules be updated inconsistently is sucky. It should be super consistent, and not depend on the render timing of the animator (since it grabs the animator's `m_time` and gets the floored frame and uses that, but it still has to depend on `m_time` which is a render-thread updated variable.).
    - If i were to redo this engine, I would definitely just make everything a fixed timestep and have rendering run right after simulation. Then when I want to separate out the rendering into its own thread, then I would choose how stuff gets divvied up, but for the most part everything is simulator-thread driven and then the render thread lerps the provided values.


### Script reform

>-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
>                SCRIPT REFORM
>-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
- Basic plan for the script reform:
    ```cpp
    auto& hecs_system{ find_service<HECS_system>() };

    // Adding a struct.
    hecs_system.emplace_struct("Struct_type",   // Struct typename as str.
                               Struct_type{});  // Struct default data.

    // Add a system (kind of like the script but doesn't belong to gameobjects/entities).
    hecs_system.emplace_system("Concrete_system_type",       // System typename as str.
                               std::unique_ptr<System_ifc>(  // Unique instance of system as ifc ptr.
                                   new Concrete_system_type())
                               { "Struct_type",              // List of required structs (checked when entity is added to system).
                                 "Struct_type_2" });

    // Add system ordering (@NOTE: if this is omitted for a struct, then it's a free for all for
    // every system using the struct; that would be the ideal for structs that aren't
    // accessed/written multiple times in an entity).
    hecs_system.add_struct_system_order("Struct_type_69",                  // Struct typename.
                                        { { "Concrete_system_type_222" },  // All system typenames that use `Struct_type_69`, placed in an order.
                                          { "Concrete_system_type_555",
                                            "Concrete_system_type_666" } });  // @NOTE: both `555` and `666` can be run at the same time, but if an entity is added to both `555` and `666` then an error occurs.

    // Creating an entity.
    hecs_system.create_entity("Entity_name",             // Entity name.
                              { "Struct_type",           // Structs allocated for new entity.
                                "Struct_type_2",
                                "Struct_type_3" },
    //                          { "Concrete_system_type",  // Systems to add entity to.
    //                            "Concrete_system_type_2" }
                              );  // ^^No more adding systems. I'm gonna try to go thru with ECS.

    // Querying for an entity.
    Concrete_system::invoke_system() override
    {
        static auto const k_query{
            component::System_helpers::compile_query_string(
                "(Struct_type_2 && Struct_type_3 && Struct_player && !Struct_deku_scrub)") };
        auto comp_lists{ service_finder::find_service<component::Registry>()
                             .query_component_lists(k_query) };
    }
    ```

- [x] Emplace everything into a `string->size_t` for the struct name for deserialization.
- [x] Attach structs ~~and scripts~~ to gameobject
    - [ ] ~~When gameobjects are added to the script execution pool,~~

- [x] OVERHAUL: Change a lot of things to use `NLOHMANN_DEFINE_TYPE_INTRUSIVE()` instead of `Scene_serialization_ifc`

- [x] @THINK: How should components like `Component_model_animator` work, where it just has a pointer to an animator?
    - It definitely feels like that's probably not the best way of doing it. Bc then it's really just something that can't be serialized.
    - [x] @FOR_NOW: Just load it up as nullptr values.
    - [x] Edit the `.btscene` files to change scripts to components (refer to new systems to see what components to include).

- [x] OVERHAUL: Create new entity pool that doesn't reallocate its size.
    - Add-removes are immediate since we're doing systems now.

- [x] OVERHAUL: Just switch to using EnTT.

- [x] OVERHAUL: Remake game_object/entity type.
    - Don't require a bunch of services upfront. Use service finder.

- [x] Write scene loader to load entities and components from disk.

- [x] Have transform graph show up in imgui game object select.
    - [x] Rename to entities window.
    - [x] Stubbed out
    - [x] Render floating entities in its own list.
    - [x] Render hierarchy entities in its own list (just flat for now).
    - [x] Hierarchize the entities in the transform hierarchy.
    - [x] Include one floating entity.
    - [x] Make nodes clickable and have them show up in the inspector
        - [x] Initial
        - [x] Have a deselect region at the bottom.


- [x] Next goal: Get stuff rendered onto the screen again~!
    - [x] OVERHAUL: Change these things into components
        - [x] Transform (serializable)
        - [x] Render object (partial serializable)
            - Needs to be built in case deformed model.
            - [x] Fix Transform gizmos!
                - [x] Position, rotation
                - [x] Scaling. <---- THIS WAS HARD!!!!!
        - Physics object (partial serializable)
            > Needs to be built depending on the physics obj type.
            - [x] Initial.
            - [x] Get a couple physics objects into the btscene file.
            - [x] Draw the debug meshes again!
            - [x] (If doable) get the selected object to be rendered as well!
                - [x] Fix picking! (Bc picking just crashes for some reason... it might just be simple as an assert on an undef func?)
                - [x] Re-add the render job to render the selected object.
                - [x] Since I added it to the debug render job, add masks for the meshes and edit it in the imgui menu bar.
            - [x] Connect the physics objects to the entity transforms!
                - This should probably just be its own system that runs after physics calculations finish. I think that would be good eh!


- [x] Inspector window.
    - [x] Create component-grabbing tech.
    - [x] Get a few components rendered in.
    - [x] Render object settings
        - DISABLE THIS WHEN `Created_render_object_reference` exists.
    - [x] Physics object settings
        - DISABLE THIS WHEN `Created_physics_object_reference` exists.
    - [x] Created_render_object_reference
    - [x] Created_physics_object_reference


- [x] Unstick physics objects.
    - If the object is kinematic or dynamic, use the move-kinematic func.
    - [x] Send a move func if trying to move w the gizmo.
    - [x] For static objects, just send an error message "trying to move a static physics object".

- [x] Fix the physics engine crashing on cleanup bug.
    - Turns out there were llingering physics objects that never got fully deleted before exiting.
- [x] Fix logger not detecting \n chars properly.


- [x] Create simple startup .toml file.
    - It'll just be 
    - Reference: https://marzer.github.io/tomlplusplus/


- [x] Ummm, ig I made the `Load Scene..` menu item completely functional.


- [x] "Play" and "Stop" button.
    - There could be an `if` statement for what systems would run with "play" on?
        > I worry that this would cause bad branching, but hey, branch prediction should figure out the pattern that something's always gonna be a certain way after a few cpu cycles right?

    - [x] Upon clicking "Play" button
        - [x] Save a copy of the current scene being edited prior to setting `s_play_mode = true;`.
            - [x] Create the implementation of `world/scene_saver.h` so that this all can get serialized and saved.
                - This may not even need to be saved out to a file. Up to you how you wanna do it tho.
                - @NOTE: Did not need to use/create `scene_saver.h`
        - [x] Clear all registrations (mainly `Created_render_object_reference`)
            - [x] Change it so that `Deformed_model`s are only allowed when `s_play_mode == true`.
                - I.e. during the level creation mode/screen, only T-pose models and stuff!!! (static models)
            - [x] Change it so that `Physics_object`s in the physics engine only get created when `s_play_mode == true`.
                - This is suuuuper important bc creation of physics objects use the `Transform` as its starting point, and then it takes over the `Transform` while it's active in the entity.
                - Since you can't really click and drag to move stuff very effectively in the level editor, this is necessary.
                - Tho, once you're playing, and you wanna just drag stuff around... does that mean you just can't anymore?
                > I think the above ^^ needs some more thought about if I'll support dragging around physics objs and how?
                    - @UPDATE: So it turns out that physics objects will get moved durng play mode as well as during editing mode, however, for static objects, since they're unable to be moved during play mode, they will be static.
        - [x] Set `s_play_mode = true;`
        - [x] Run!
            - When running, everything that needs to get created (e.g. render_objs, phys_objs), will still get created in the systems that create them, but this time, since `s_play_mode == true`, then everything will get created completely!
    - [x] Upon clicking the `Stop` button, the previously saved file is loaded again from disk, after unloading all scenes.


> Some thoughts on how there could be parallelization.
>   Simulation running on one core, and then rendering running on another core, with that bit of sync when adding/removing render objects and passing transforms sim->rend could be good.
>   And then the remaining cores are worker cores. They're asleep unless there are barriers submitted. They all work to tackle all the jobs in each barrier to finish whole barriers as fast as possible (first come first serve as far as priority).
>
>   A system can submit a barrier of jobs (i.e. barriers must be done in order, but the `n` number of jobs in the barrier can be in any order).
>     After submitting the barrier of jobs, the system will work on the submitted barrier too (when calling on the `barrier.wait()` func), so that there's progress being done on everything at the very least.
>
>  There could be more parallelization happening (e.g. having another thread dedicated to some other type of work) (e.g. figuring out what systems depend on each other and running only the ones that depend on each other in order)
>
>  Thankfully, I should just implement this single-threaded now first and then profile and think about multithreaded workflows later.
>    Even tho I know it's a very special interest of mine.




- [x] Achieve feature parity as before.
    - [x] Player character
        - [x] Input getting system.
        - [x] Char-con collide-n-slide algorithm system.
        > @NOTE: The above ^^ is still in the coupled mode as before, but there are clear sections and is a bit more laid out better. Hopefully this makes decoupling this in the future a bit easier.
        - [x] Writing the animation to entity transform rotation.
            - [x] Add required componenet to player obj
            - [x] Fix the asserts put up and implement.
        - [x] Fix camera not following an object.
        - [x] Documentation.
            - [x] Remove `struct Character_mvt_state`'s `m_` variables.
            - [x] Add docstrings for anonymous namespace funcs in `player_character_movement.cpp`
                - [x] Remove @TODO banners too
    - [x] Animator editor.
        - [ ] ~~Umm, whatever is needed here.~~
        - [x] ~~Change `animation_frame_action_tool/editor_state.h` into a component.~~
        - [ ] ~~Create system that uses the new component.~~
        - [ ] ~~Plug component results into imgui system.~~
            - ~~@NOTE: This does ~~
        - [ ] ~~Delete `animation_frame_action_tool/editor_state.h`~~
        - [x] Remove the stuff from `editor_state.h` from the newly created component for the anim frame action editor.
            - [x] Just have the component have a flag for whether to reset the editor state. (True as default).
                > So then every time we reload the editor tab, it will reset the editor state.
        - [x] (free space) Have the imgui stuff still connected to the original `editor_state.h` (i.e. do nothing)
        - [x] Create system that uses component to reset editor state but other than that just uses the editor state same as before (also adds and changes components and stuff).
            - [x] Make switch for forcing allowing the deformed rend objs (needed for editing animations obviiii)
            - [x] Stub out the system
            - [x] Initial try to get a scrubbable animated character in there.

            - It's not working so more work needed???
            - [x] Figure out why there's no rendered render obj.
                - [x] Fixed for if you select a different model to edit.
                - [x] Fix render obj not getting properly creating from the get go.
                    > Turns out when the level for the editor got loaded in the editor agent immediately just deleted everything (_dev_animation_frame_action_Editor.cpp).
            - [x] Fix ctrl data points not getting loaded w the data types.
            - [x] Fix hitcapsules not being attached to bones of animator.
                - [x] Also enable/disable hitcapsules from here too.
            - [x] Fix imgui timelines not getting changed out.
                - [x] Half done.
                - [x] Fix the actual timeline regions not showing.
                    - Do you think it's just not configured correctly in the .btafa?
                    > THE ISSUE: The order of the .btafa and the .btanitor needed to be the same. So there needs to be a map from the state idx of the .btanitor to the .btafa region idx.
                - [x] Fix default/first timeline that gets loaded being empty/0 and also not plain working.
                - [x] Fix the random crashing when closing the program.
                - [x] Fix the random crashing when switching AFAs. Or discarding AFA changes.
                    - It just seems like there's no more animator and that's an issue type thing? It might have to be manually done in.
                    - When the AFA gets saved, it replaces the AFA in the AFA bank, so it unfortunately messes up the pointer that's in the model animator.
                        - The bank doesn't get edited normally, but I had to check whether the pointer to the AFA changed and then reload the AFA into the model animator in `_dev_animation_frame_action_editor.cpp`
                - [x] Fix crashing when closing the program after saving an AFA and then closing the program
                    - My guess is that registered hitcapsule sets in the overlap solver are getting held up in there.
                    - @SOLUTION: Needed to remove the old AFA's hitcapsule group set from the overlap solver.
                - [x] Rename `s_selected_timeline_idx` to `s_selected_afa_idx` along with other "timeline" names/labels that shouldnt be this way (imgui_renderer.cpp:661)
            - [x] SMOL: Just add a cleanup statistic for number of group sets in the solver leftover.
            - [x] Fix vars of AFA data viewers. It appears the bools are flickering between false and true when over the override region???
                > It appears to be working correctly now that there's the right stuff loading in from the system???? Idk double check pls.
                > Huh... it really doesn't appear to be flickering anymore...
                - After doing some fiddling, it just seems to be the animator updating since even tho `_dev_animation_Frame_action_editor.cpp` sets the time every tick for the simulation, if the renderer is slower or something, it will update the animator forward and then the next frame is shown.
                    - So the solution: stop the animator from ticking forward in time (if there's a speed indicator or something????)
                        - [x] Fixed this problem. (Redo if the weaker hardware is still having issues)

            - Once that is done...
            - [x] Implement processing controllable data.
            - [x] Think about how to load and process controllable data inside of the regular animator too.
                - Honestly, the same way that's going on w the other handles I think that will just work haha.
                - Basically, get the data handle references you need, then query them
                ```cpp
                if (my_comp.some_event == nullptr)
                {   // Get information.
                    my_comp.some_event =
                        &my_animator.get_anim_frame_action_data_handle().get_reeve_data_handle(
                            some_event_label);
                }

                if (my_comp.some_event->check_if_rising_edge_occurred())
                {
                    // PROCESS EVENT HERE.
                }
                ```

                - WARNING: The animator should run in the simulation loop for handling the events, so that there's no need to rely on the renderer for updating the animators.
                    - [x] Do this. For now, have animator get updated ~~_only_~~also in the simulation loop.
                        - [x] Update the hitcapsules and AFA data.
                        - I basically just added a new timer for the renderer, and made a profile switcher that updates the correct timer for the animator.
                    - [ ] ~~Make a later task to have the renderer interpolate the animations. (defer this task but make sure you're not forgetting about it!!!!)~~
                        - It was actually easier to just make it now! So I did.
                    > @NOTE: I believe that there should be testing that these values don't get off, but theoretically they never will!!
                        > Mmmm, this is probably gonna bite me in the butt in the ftuure??
            - [x] Fix crashing when discarding/saving an AFA controller (since the split animator now caused this issue to arise again)
                - Basically the animator needs to be reconfigured _after_ the renderer does its render step. Add another system check to check if the working AFA changed.
                - My fix is basically just calling the AFA editor system _again_ after doing the renderer's render pass.
                    - Hacky but oh well, problem solved I think.
            - [x] Have a component that configures the model animator with an AFA controller.
            - [x] Run a system _right after_ the system that created model animators to configure the model animator to add the AFA controller.
                - Also make sure that this is before the animator gets updated in the `animator_driven_hitcapsules_stuff_stuff()` system.
                - [ ] ~~Add an assert that if the model animator isn't created yet then that's a SERIOUS issue, bc it should've JUST been created just prior. (also leave a message for future me pls!!)~~
                    - I don't think this is necessary bc I just added a branch into the render object lifetime system instead of creating a new system.

- [x] Now do the thing I initially wanted to do but couldn't
    > [ ] Create ~~script~~system for character controller movement but without the input from player.
        > [ ] ~~Perhaps this could just be a script that sends its data to another script? Maybe there could be a connections type thing? Mmmmm maybe provide a `on_start()` func for scripts where they could get the script that they need to send to and stuff?~~
            > This all got solved with the script to ECS overhaul.
    - [x] Split player movement into two systems.
        - [x] Stub out system split.
        - [x] Get player input.
            - [x] Make between component, `Character_controller_input`.
        - [x] Do movement.
        - [x] Make this work for the player!
        - [x] Make another non-player character that has simple movement.
        - [x] Test it with an imgui edit.

- [x] Delete the old system. Gut it out.
    - [x] Ensure that saving the scene actually saves the scene.
    - [x] Remove the debug draw lambda from the renderer.

- [x] Make merge request.
>-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


## (cont./unblocked) Use simple anim dude and make dummy character and create fighting animation movesets.

- [x] Add another dummy character that plays the same anim as player.

- [x] Write the hitcapsule group sets interact w each other.
    > @NOTE: This is kind of the order of checking I think would be good for the interaction.
    - [x] Cancel check if same hitcapsule group set.
    - [x] Cancel check if hitcapsule group set A does not have any enabled hitcapsule (give hurt).
    - [x] Cancel check if hitcapsule group set B does not have any enabled hitcapsule (receive hurt).
        - @NOTE: For iframes, the animation should disable the receive-hurt hitcapsule.
        - @REPLY: Or wait, would a different method be better? For debouncing hits, bc that's needed too tho.

    - [x] Create bounding sphere for both interacting sets (set A's enabled hitcapsule (give hurt), and set B's enabled hitcapsule (receive hurt))
        - For each set of hitcapsules:
            - Gather all `calcd_origin_a` and `calcd_origin_b` and find the midpoint.
            - Find the radius of the bounding sphere by taking each capsule, taking the distance of each origin, adding `radius` to the distance, and then finding the max of the radius-included distances.

    - [x] Compare the bounding spheres to see if they collide. Cancel if they don't. Continue if they do.

    - [x] Check each individual hitcapsule in set A with all of the others in set B. `O(N^2)` unfortu (^_^;)
        - For each hitcapsule pair, create their respective bounding sphere, so for each one:
            - Find the midpoint between `calcd_origin_a` and `calcd_origin_b` for the bounding sphere.
            - Find the radius of the bounding sphere with:
                ```cpp
                bounding_sphere_radius = (glm_vec3_distance(capsule.calcd_origin_a,
                                                            capsule.calcd_origin_b)
                                          * 0.5f)
                                         + capsule.radius;
                ```
        - Then if the bounding sphere check succeeds, check the actual real capsule-to-capsule check.
            - @TODO look up a reference for this!!!!!

    - [x] If the real capsule-to-capsule check succeeds, then `break;` and show that the overlap occurred.
        - This is where the callbacks or event for "got hit" would get triggered or something.
        - Mmmmm maybe the callback would be as the form of both game objects having a `script_hitcapsule_processing` script, both those scripts get searched and found, and then for set A's game obj's script, the "you hit a hitcapsule, deal hurt" func gets called, with set B's game obj's script supplied as a parameter so that inside that "deal hurt" func it calls set B's script's "i got hurt" with all the hurt stats, with set A's script, in case set B's script calls an "i was parrying" call.
            > This interaction could get simplified with data, but maybe having scripts could be good... ahh but having a way to send messages like this is gonna be kinda hard I think. The way described above would work if the `find_script()` func was sophisticated and could find a script attached to the game object that has a certain interface tho.
            >
            > Maybe then "interface groups" would be a useful thing to attach to scripts? Or like a script tag system? And `find_script_by_tag()` or something.
        - [x] I thought I did it.
        - [x] For some reason the capsule-capsule collision isn't quite working right? Investigate this.
            - Yeah so basically none of these algos work. I feel like Joseph Smith rn.
            - [x] Try this method: https://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments (Fnord's answer)
                - This one works!!!

    - [x] Create the attacking interface.
        - Maybe as a system that runs after `update_overlaps()` runs.

        - [x] Have the hitcapsule overlap solver return which overlaps happened and what entities attacked what entities in a vector.
            - [x] Should return Entity UUID of the offender and defender.
                - [x] Stubbed it out.
        - [x] Create `component::Base_combat_stats_data`
            - Has these data:
                - i32 dmg_pts;
                - i32 dmg_def_pts;
                - i32 posture_dmg_pts;
                - i32 posture_dmg_def_pts;
            - @TODO: @FUTURE: In the future have some values in the animator that cause higher damage and higher defense in the form of multipliers to these base numbers!!
        - [x] Create `component::Health_stats_data`
            - Has these data:
                - i32 health_pts;
                    > If this reaches 0 then the entity is destroyed.
                - i32 max_health_pts;
                    > Max HP.
                - i32 posture_pts;
                    > If this reaches `max_posture_pts` then the entity is posture-broken.
                - i32 max_posture_pts;
                    > Max posture pts.
                - float posture_pts_regen_rate;
                    > Rate per second of posture pts regeneration.
                    > @NOTE: This value should also be scaled by the animations (e.g. if a guard is held, that causes the posture to regen quicker. )
                - bool is_invincible;
                    > This completely negates and causes no damage to be taken.
                    > Usually only marked this way for props and such.

        - [x] Get offender/defender ids and components into the system.

        - [x] Implement processing attack logic.
            - See below for an example.
            - Use time debounce.
                - [x] Add time field to combat.
                - [x] Move time field to health stats component.
            - [x] Implement handling of parrys/guards.

        - Below is some example code I thoguht of for processing attacks.
        ```cpp
        enum Offense_type
        {
            OFFENSE_SLASH_HIT,
            NUM_OFFENSE_TYPES
        };

        struct Offense_snapshot
        {
            Offsense_type type;
            int32_t dmg_pts;
            int32_t posture_dmg_pts;
            int32_t posture_dmg_def_pts;
        };

        enum Defense_type
        {
            DEFENSE_NONE,
            DEFENSE_GUARD,
            DEFENSE_PARRY,
            NUM_DEFENSE_TYPES
        };

        struct Defense_snapshot
        {
            Defense_type type;
            int32_t dmg_def_pts;
            int32_t posture_dmg_def_pts;
        };

        struct Attack_hit_result
        {
            struct Result_data
            {
                int32_t delta_hit_pts{ 0 };
                int32_t delta_posture_pts{ 0 };
                bool can_enter_posture_break{ false };
            };
            Result_data defender;
            Result_data offender;
        };

        Attack_hit_result process_attack_hit(Offense_snapshot const& oss, Defense_snapshot const& dss) const
        {
            Attack_hit_result result;

            int32_t dmg_pts_real{ oss.dmg_pts - dss.dmg_def_pts };
            if (dmg_pts_real < 0)
            {
                BT_WARNF("Dmg pts real is <0 : %i", dmg_pts_real);
                dmg_pts_real = 0;
            }

            int32_t posture_dmg_pts_real{ oss.posture_dmg_pts - dss.dmg_def_pts };

            int32_t posture_dmg_pts_sendback{ (oss.posture_dmg_pts - oss.posture_dmg_def_pts) * 0.5 };
            if (posture_dmg_pts_sendback < 0)
            {
                BT_WARNF("Posture dmg pts sendback is <0 : %i", posture_dmg_pts_sendback);
                posture_dmg_pts_sendback = 0;
            }

            switch (dss.type)
            {
            case DEFENSE_NONE:
                posture_dmg_pts_sendback = 0;
                result.defender.can_enter_posture_break = true;
                break;
            case DEFENSE_GUARD:
                dmg_pts_real *= 0.5;
                posture_dmg_pts_sendback = 0;
                result.defender.can_enter_posture_break = true;
                break;
            case DEFENSE_PARRY:
                dmg_pts_real = 0;
                posture_dmg_pts_real *= 0.5
                result.offender.can_enter_posture_break = true;
                break;
            }

            // Write results.
            result.defender.delta_hit_pts     = -dmg_pts_real;
            result.defender.delta_posture_pts = -posture_dmg_pts_real;
            result.offender.delta_posture_pts = -posture_dmg_pts_sendback;

            return result;
        }
        ```

        - [x] Fix posture damage bug.


- [x] Create animator that can change animator states for attacks.
    > First thing to do here is to think about the design for this.
    - [x] Created variables and state transitions.
        - [x] Initial
        > Yknow, it seems like there's no real animator component, so it's gonna be tough trying to make an imgui panel for it. Ig I could just use the `created_render_object_reference` component to do that tho.
        - [x] Make the imgui panel insdie the `created_render_object_reference` component.
        - [x] Implement `find_animator_variable()`.
        - [x] Test it.
    - [x] Connect the animator to the character movement system.
        - [x] `is_moving`
        - [x] `on_attack`
        - [x] Connect the char mvt anim state component to the actual animator.


## Animation root motion.

- [x] Find root bone.
    > This bone will have the name "Root" (case insensitive).
    - [ ] ~~If there are bones/joints and there is _not_ a "root" bone, then error out.~~
    - ~~Or maybe just look for all bones that don't have parents and select one or if multiple select the one that has the word "root" in its name.~~
    - I just ended up choosing the first bone that was in the `joints_sorted_breadth_first` list as the root bone.

- [x] Find velocity at every frame.
    - [x] Delta position from current frame to next frame.
    - [x] ~~For last frame, get as far to the end as possible.~~
        - Perhaps this needs a different method for the last frame?
        - DIFFERENT METHOD: I ended up just averaging the delta root pos of the first and second-to-last (could be the same as the first) frame.
            - I think this is a good approx. If there's more need to do more, then I'll redo this.
    - [x] TEST: make sure that the values are correct for delta pos.

- [x] Change animator to use root motion or not.
    - [x] Have a `bool use_root_motion` in the animator.
    - [x] Do this for rendering:
            ```cpp
            calc_joint_matrices(time, loop, use_root_motion, joint_matrices);
            ```
            @NOTE: All it does is make the root bone be (0,0,0) position.
    - [x] Do this for simulation:
            ```cpp
            get_joint_matrices_at_frame_with_root_motion(time, loop, root_motion_delta_pos, joint_matrices);
            ```
            This makes the root bone be (0,0,0) position but also returns the delta pos for the root motion.

- [x] Include movement with root motion.
    - [x] Search for the `component::Animator_root_motion` component inside the entity when doing the `input_controlled_character_movement()` system.
        - [x] If exists, use it for the movement, and use the world space input for turning and facing angle stuff.
            - [x] Initial separation of movement velocity and world space input for turning.
            - [x] Fix speed issue.
                - [x] Write imgui for the component.
                    - I'ts really consistent. Just `0.081`. Idk if this is the right speed or not.
                - So um, it turns out that the `*2` hack made the animation root motion twice as fast... and times one... it's correct even tho it looks wrong.
                    - I did math to verify, and then looked real closely at the feet and the grid lines and yup it's correct.
            - [x] To make look better, zoom in the camera.
            - [x] Grab the AFA data of `turn_speed` from the animator's AFA.
                - Aaaaaa I wish this weren't _inside_ the animator like this 😭
                - There is a refactor below that should help sooo much for all of this.
        - [x] If doesn't exist, then use world space input for both movement and turning.
            - I.e. the current solution (well, not after this change).
        - [x] BUG: Fix the one bug where the running animation gets stuck after the attack animation. It's kinda weird?
            - Ok the bug appears to happen when it's a turnaround angle-level turn while doing the attack animation, and the char cannot turn around.
                - Ok, it also just happens while in the running anim. Nothing needed here!
                - It looks like the character just literally can't turn around for some reason.
        - [x] Make Grounded not just use the magnitude. Have it use the actual root motion delta pos but rotated from the facing direction.
            - [x] Initial.
            - [x] Fix attack anim root motion not working.
                - It (I think) is bc `grounded_state.allow_grounded_sliding` is false.
    - [x] MISC: Some tuning (now it's like KUSR).
    - [x] MISC: Make transitions easier by doing a many-to-one system for from-to-anim transitions.
    - [x] Figure out midair movement.
        - [x] @THINK: Should multiple anims be working together for this in a blendtree or should it be some kind of AFA data control where it changes the mode of control?
            - I mean, I think having an AFA control to change the mode makes sense and would be good.
            - I think yeah having the mode switching would be best, especially if could adjust some parameters like drag and friction and acceleration/deceleration from the AFA data too. That could make some really good ice animations probably?
                - For now tho it would just be best to have the bool for doing freeform movement.
                - Also have a float value for accel, decel, and max speed.
        - [x] Add jump and fall anims.
        - [x] Allow turning while midair.
            - Immediate for jump.
            - 7.5 for fall.
        - [x] Add these AFA data:
            - bool use_mvt_input
            - float mvt_input_accel
            - float mvt_input decel
            - float mvt_input_max_speed
        - [x] Connect AFA data to movement system.

- [x] Separate root motion update from updating hitcapsule positions. (To fix the 1 sim-tick lag)
    - [ ] ~~Put root motion fetch after player input and before input_controlled_char_mvt()~~
        - Nothing special happens in player input really, and other things that set the animator vars happen in the `input_controlled_char_mvt()` so it just needs to happen before player input and char mvt.
            - [x] Do ^^ above ^^
    - [ ] ~~Leave hitcapsule positions in same place (right before hitcapsule overlap check).~~
    - So ig the `write_to_animator_data` stuff will be lagging one sim-tick but I think that's fine?
        - _Something_ has to lag, and having the movement of the current frame of the animation show up is most important imo.
        - ~~So the order should be:~~
            - 
    - [x] Add missing `get_root_motion_delta_pos()` func.
    - [x] Complete the reordering.
        - It works!!!
    - [x] rename `system::set_animator_variables()` to something like updating the animator vars, animator, and writing the root motion things.
    - [x] combine `get_joint_matrices_at_frame()` and `get_joint_matrices_at_frame_with_root_motion()` similar to `get_joint_matrices_at_frame()` with the `bool root_motion_zeroing` param.


## Have CPU character attack, and have there be guard, parry, hurt -type interaction.

- [ ] Connect attack results to new animator.
    - [x] Report the attack results to the animator (somehow).
        - I just piggybacked off the `Character_mvt_animated_state` struct created in the last task!
    - [x] Fix not getting the right reference for the `Character_mvt_animated_state`
        - Ok so now I can say that I tested this. It works as far as it should.

- [ ] Create knockback anims in blender.
    - [x] Hurt.
        - [x] Hurt_receive
        - [x] Hurt_receive_strong
    - [ ] Parry.
        - [x] Parry_left (when attack comes from left)
        - [x] Parry_right (when attack comes from right/up/down)
        - [ ] Parry_strong (drag sword in ground)
    - [ ] Guard.
        - [x] Guard
        - [ ] Guard_strong (reel/stumble back but stay on two feet)
- [ ] Create other anims in blender.
    - [x] Ready parry
        - [x] From idle/walk/attack
        - [x] From running
    - [ ] Hold guard (slightly more relaxed than the parry anim)
        - [x] Idle
        - [ ] Slow walk
            - [x] Try a root motion speed.
            - [x] Figure out root motion speed.
                - I think the 3 m/s is a good speed.
            - [ ] Create slow walk anim.
                - [x] Make a hilarious very wiggly walk anim.
                - [ ] Make a better one later.
- [x] Add knockback anims into the .btanitor and .btafa
    - [x] Hurt_receive

- [x] Add a bunch of animations to .btanitor and .btafa for guarding and readying parry.
    - [x] Fix turn speed issues with guard-walk.

- [x] Make deceleration on "ready_parry_from_running" not as fast.
- [x] Make transition from "ready_parry(_from_running)" to "guard_idle" quicker (the lowering the sword part).

- [x] CHECK: If anim doesn't exist when creating the btanitor, crash the program.
- [x] CHECK: If .btafa anim state doesn't exist in .btanitor, crash the program.

> Mmmm so I do think that there's just no good way to move 

- [x] When receiving hurt, have char face offender.
    - Hopefully this will get the problem solved with defender going a diverging direction.
    - Maybe there needs to be an anim for getting hurt from behind?
        - Yeah, bc guard blocking doesn't work from behind so just this is needed.
        - And then if got hurt from behind, in this case too turn defender to face 180deg away from offender.
    - [x] Initial.
    - [x] Create getting hurt forward anim.
        - Bc getting hurt back anim goes the wrong direction when played (w/ root motion).

- [x] Add shape-in-shape for character virtual controllers (all the ones currently) so that they collide with each other and can't just walk thru each other.
    - This helps a lot with reaction stuff bc now chars don't go thru each other.

- [x] Adjust camera to make similar to KUSR.
- [x] Add lock on for camera.
    - It doesn't look quite as good as I was hoping, but it's a good start. It works.
        - Vertical stuff kinda sucks.
        - Also, the player can still attack in different directions from the camera direction, which I want to prevent!!! (But more work is required for this to happen).
    - [x] BANDAID: Makes a simple component that can be locked onto w a y_offset value too.
    - [x] BANDAID: Rotate down the camera a bit (marked w/ `@HARDCODE`)

- [x] Change knockback anims to use -1.0 knockback distance, but have offender's AFA data send what length of knockback to defender.
    - So it's kinda interesting. I played some sekiro and some kannagi usagi to see how the knockback stuff works there and it seems like it just doesn't happen when making contact with an enemy?
        - Or maybe it's when the player is in range of an enemy? Idk really.
        - At the very least, I think having a basic shape-in-shape thing going on with the character controllers is needed. (adding task above)
    - [x] Get the `afa_data.send_root_motion_multiplier` -> `afa_data.root_motion_multi` -> `component_root_motion.root_motion_multiplier` cycle in there.
    - [x] When hurt anims finish moving, reset the `root motion multiplier`.
        - This needs to be done in the AFA timeline.
        - I could do this by just not pressing play button.
    - [x] Fix the AFA editor view not showing the root motion (i.e. the anim stays put in the editor and this is not wanted)
        - Ok so it's a sometimes it's working and sometimes it's not????
        - It looks like it works if you never hit play mode for some reason???
            - As soon as you hit play mode once, then it stops working.
        - ROOT CAUSE: Turns out it was editing a reference of a joint anim frame. After realizing that (alarm bells should've been ringing when I was doing a `const_cast` on a freaking reference!!)
    - [x] Actually make the length of the knockback and stuff to 1.0

- [x] Make attack only possible forwards when camera is locked on
    - [x] Create strafing anims.
        - [x] Just do the root bones for trying speed w/ root motion.
        - [ ] ~~Make east/west ones be same speed as north one.~~
            - I decided against this after getting the magnitude preservation in root motion stuff.
        - [x] Create actual anims (for running)
            - [x] East
            - [x] West
            - [x] South
        - [x] Create actual anims (for guard-walking)
            - [x] East
            - [x] West
            - [x] South
    - [x] Blend between strafing anims with a blend tree.
        - Have back moving one be slower since walking backwards.
        - I think just the 4 directions is fine? Maybe there just needs to be a thing in the animator that uses two AFA data as the horiz and vert mvt input, and then it will combine the animations?
        - [x] Create the animator template for blendtrees.
        - [x] Import it correctly.
        - [x] Assign values to `mvt_facing_angle`.
        - [x] Blends animation correctly with the animator.
    - [x] Face player forward all the time, even when moving (except for sprint moving if this is a thing).
        - FOR NOW: Just make sprinting not a thing.
        - [x] Faces visually
        - [x] Override the facing direction and its input for the desired facing direction as well.
            - Now that this is implemented this should be the foundation for the blendtree stuff.
        - [x] Write the facing angle value to animator data.
    - [x] Test that the anim blending is working correctly (after creating actual anims)
        - Diagonals kinda have the same issue as the root motion where the magnitude is not preserved, but this isn't something I'm gonna worry about for now.
    - [x] IMPROVE: Keep magnitude of root motion vector, instead of linearly interpolating.
    - [x] Delete dead code (`#define DELETE_ME`)
    - [x] BUGFIX: Still face forward even if doing attack over and over (currently doesn't reset to facing the target).
    - [x] Smol perf timer for simulation and rendering just to see what was going on.
    - [x] Add strafing anims to guard-walking.

- [ ] CPU behavior. (I don't wanna say AI anymore)
    - These are the CPU states I imagine:
        - UNAWARE (watching, patrolling, etc.)
            - If attacked or gains awareness of its enemy (by sight or sound or aura or something?), then will enter AWARE mode.
            - If sees/hears/feels a sense of suspicion, then will enter SUSPICIOUS mode.
            - It does seem like having some kind of SUSPICIOUS state is necessary tho, bc then the CPU doesn't just go super-chill after it loses sight of its enemy.
        - SUSPICIOUS (searching for its enemy, only aware of the position of suspicion)
            - CPU will typically investigate the position of suspicion.
            - If the CPU happens upon its enemy (gains awareness), then will enter AWARE mode.
            - If no gaining of awareness (no line-of-sight, etc.) after X time, then will return back to UNAWARE.
        - AWARE (has definite sight of its enemy, "attack mode")
            - ~~Keeps track of last known position of its enemy. If has line-of-sight of its enemy, then will update its last known position of its enemy.~~
                - I think if it just "knows" where its enemy's position is at all times that's good enough and will make it easier and not have to worry about losing line-of-sight for programming the CPU.
            - If CPU loses line-of-sight for X time, then will switch back to SUSPICIOUS.
            - Will move to be in range of melee, or if a ranged CPU, will fire ranged weapon unless its enemy is in range of melee.

    - [ ] Create CPU behavior component.
        - [x] The struct for its enemy awareness.
        - [x] Query the position/location of the head from the char model for eyesight.
            - This way looking around can be accomplished by an actual looking around animation and doesn't have to be programmed.
            - Perhaps could be similar to the hitcapsule-to-bone system?
        - [ ] ~~Checkbox of "player character is an enemy".~~
            - There needs to be a midpoint for the player character that the origin is, since the player origin is at the feet at the moment oof.
            - Crossed out. Using a mask now.
        - [ ] Current state text.
        - [ ] Param of viewable fov and radius for line-of-sight awareness.
        - [ ] Param of hearable radius for hearing points of suspicion.
        - [ ] Param of viewable fov and radius for seeing points of suspicion.

        - [x] Draw lines showing eyesight and CPU suspicion/awareness detection zone.
            - You can create new color codes and stuff for this too.
            - [x] Double check that using the "Eyes" bone for tracking eyesight is working correctly.
            - [x] Detection zones.
            - [x] Lines drawn from CPU to possible enemies.
                - If too far, draw the extra distance as red, and the requisite distance as yellow.
                - If close enough, draw the whole line as green (or whatever color wanted?)
            - [x] Fix detection zones. Now they detect the enemy in them.
            - [x] Utilize the `my_enemy_bitmask`.
            - [x] Write awareness value into `CPU_enemy_awareness` component.

        - [ ] Movement behavior that reads from the detection runtime data (different system than `cpu_character_enemy_detection.cpp`).
            - [x] UNAWARE.
                - Preprogrammed thingy. Just stand there for this POC.
            - [ ] SUSPICIOUS.
                - Preprogrammed thingy. Have a speed for going to the suspicious point of interest, and play an animation.
                - [x] Special approaching/walking animation.
                    - Have anim looking left and right while traveling to the point of interest.
                    - [x] Initial just the normal running anim.
                    - [ ] Create walk animation in blender.
                        - [x] Ichiou
                    - [x] Try just triggering into this instead of immediately running.
                    - [x] Don't run on the change-state frame. (since the trigger + move is executing on the same frame but need it to lag behind one frame so that new BTAFA properties get read in.)
                        - This is so that the char doesn't immediately turn towards the suspicion point, but rather faces where they are currently for the first part.
                    - [x] Fix an issue w `st_awareness_suspicion_walking` turning into `idle` in the single frame when there's technically no moving, since the last frame of `st_awareness_suspicion_begin` has `can_move` set to `false`.
                        - [x] Tried a workaround by setting `can_move` to `true` in the final frames.
                            - IMO, it's a bit too complicated and corner-casey, so I think it would just be better if I made it allowed to have multiple cases for evaluation in the `"condition"` script part.
                                - ~~Well, honestly, I could probably just work around this by having a `is_suspicious_and_moving` flag, where the end of it is it off.~~
                                - The ^^ above ^^ would be a no. It's just not a good setting at all. It would be too confusing that way.
                                    - But also, the double condition wouldn't work either...
                                    - So cross out the logical AND task below.
                        - [x] Put trigger for when the walk-over/search is done and the final look-around should be done `on_suspicious_walk_end` (or smth).
                            - [x] ACTUALLY MAKE IT A BOOL. Since triggers have the possibility of getting eaten.
                        - [x] Put event at end of animation to let behavior know to go from SUSPICIOUS to UNAWARE detection state.
                        - [x] Read the return-to-UNAWARE event and go back to UNAWARE.
                    - [ ] ~~Make a state transition that will be something like `on_running and is_suspicious eq true` w/ the regular running anim being `is_suspicious eq false`.~~
                        - [ ] ~~Logical AND in state transition logic.~~
                        - @NOTE: This wouldn't work, bc of the `is_moving` bug. Stopping for the final look-around at the end of the walk/search should be done by a trigger.
                            - And then use an event from the AFA to call back to when the "suspicion" should return to "unaware".
                - [x] "realize suspicion" animation, where there's a 0.5s window where the CPU goes from relaxed/unaware to alert, weapon-readied, and then starts turning around to look.
                - [x] When arriving and there's no enemy, look around and make sure that the enemy isn't out of view (but since there's no leads for another place the enemy could be at, then don't look in another place).
                    - Probably there needs to be a way for the CPU to realize another place to look is what to do.
                        - FOR NOW: Do not implement this. No new leads when in SUSPICIOUS state.
                    - [x] Make animation.
                - [x] Fix not returning to UNAWARE animation.
                - [x] Fix not going to some AWARE animation.
                    - Since one doesn't exist, just use the attack animation.
            - [ ] AWARE.
                - [x] Ichiou: Have attack anim repeating and just go after CPU's enemy.
                    - [x] Fix missing hitcapsules and missing guard/parry frames.
                    - [x] It seems like there's frames where the hurtcapsules aren't hitting properly?
                        - It just seems like the sending knockback needs to be halved and then that'll fix it?
                        - Lengthening the time for the hurtboxes to be active could also help too.
                            - Bc they just don't seem to be hitting at all???
                            - Honestly this doesn't seem to be the move. Closing in on the char's enemy seems to be the move.
                                - It just seems like there needs to be a more closeness.
                        - @NOTE: After studying sekiro's animations, the char will often close in on its enemy right before or slightly during the first little bit of its attack. This ensures that it's right in the face of its enemy.
                        - [x] Have char not hit enemy so far in knockback.
                        - [x] Have char close in on enemy when starting attack.
                            - I think ^^ this ^^ will help out the most since it seems like the char only closes in enough by the end of the attack, not the beginning or middle.
                            - THOUGHT: After doing this this definitely helped out a lot w feel, tho it feels like not doing the knockback as much would also help out a lot too.
                        - CONCLUSION: So it seems that it's a lot better for the attacks and they connect consistently now. I think that it's a lot better now, but something doesn't feel right? It's like the responsiveness of the animator ig??? Idk.
                            - [x] Just check real quick that there's a good order for the input->write_animator_vals->update_animator
                                - So it's update_animator->input->write_animator_vals.
                                - It's BECAUSE it needs the AFA values that come from the `update_animator` for the input stuff.
                                - AAAAAGGGHHHH I hate how circular this is. I have to compromise which one is going to come first.
                                    - Well, having the AFA values available seems like very necessary, so this seems like a good order.
                                    - (But then what can I do to make things feel more fluid???)
                    - [x] The feel of the parrying and stuff doesn't feel right (from player end). Why is that?
                        - Probably because there needs to be a way to cancel out of the ready-parry animation so it's a bit more responsive.
                            - So then redoing a ready-parry is faster and doesn't feel like ER nightreign's sluggishness with the executor.
                        - Made the parry animation as short as KUSR. That helped the feel out a looot.

                - [x] Make change to state machine transition conditions to allow `and` keyword.
                    - Now going from a hurt to a ready-parry is possible! It feels kinda different tho ngl for some reason.

                - [x] Buffer overlaps of hurtboxes/hitboxes so that they lag by 1 or 2 update ticks (to compensate for the animator-input circular relationship).
                    - Just PROMISE me that the simulation tick isn't gonna run any faster than the allocated 16.667ms. (Bc then that would mess up real world timing)
                    - OH YES this needs to be included. It just doesn't feel like it's spot on (bc it's too early).
                    - [ ] ~~Also buffer the other direction too. Extend "parry" and "guard" frames 1-2 frames beyond what the AFA dictates.~~
                        - Just to make sure that things feel like it responds to player input.
                        - INSTEAD: FOR NOW: Just extend the AFA by 2 frames. This should compensate enough properly.
                            - [x] Do.
                                - If anybody complains about the timing, make it more lax here too.

                    - I tried doing this inside the animator, but (on top of it not working for some reason), there's an issue with writing the root motion from the hits.
                        - This needs to be done inside the actual hitcapsule interactor.
                        - [x] Do.
                            - Ok this definitely helps a lot with the feel and timing. 2 ticks doesn't feel too forgiving but yeah.

                - [x] Change inside shape for char con to be 45deg rotated box inscribing the char con box.
                    - I think having capsules would be ideal, but for combat _and_ platforming, having an inscribed box would be better.
                    - 感想：It just felt way too boxy, not smooth at all. Having a 45deg region was kinda nice tho.

                - [x] Change to capsule shape.
                    - This seems a lot better, but I need to figure out a better situation for the platforming.
                    - Like for example, falling needs to get better, where if the angle is 45deg or more it just falls off immediately instead of slowly sliding sliding sliding and then fall off.
                    - Also, for something like having ledges you don't want to fall off of, have a walking machine algorithm.

                - [x] Enter awareness animation (something like drawing out a sword, or an aggression show).

                - [x] Attack cancels.
                    - If CPU's enemy deflect/parries a certain attack, the sequence is stopped. Usually the last attack of the sequence, but could also be stopped by mikiri in the middle (think kensei isshin).
                    - The attack cancel should contain enough time for other enemy to do a counter attack.
                    - [x] Deflect cancel animation.
                        - Does this have to be the same animation for every attack chain??
                        - [x] Create anim in blender.
                        > @NOTE: The point of this is to give player char extra time to do a counter attack.

                    - vv FUTURE vv
                    - [ ] ~~Mikiri cancel animation.~~
                    - [ ] ~~Stomp cancel animation.~~

                - [ ] ~~If attack canceled, then don't have defender knockback.~~
                    - Actually, this shouldn't be the case I think.
                
                - [x] When hit by its enemy, CPU is made aware of that enemy.
                    - Maybe just turn CPU towards its enemy all the time.

                - [ ] Detect these events from CPU's enemy. (Do inside `cpu_character_enemy_detection.cpp` since it's something all CPUs should have detection of (ig even "shy" NPCs could use this information!))
                    - [ ] ~~Enemy attack -> 1st CPU guard, then 2nd CPU deflect, then counter attack.~~
                        - [x] Create new hitcapsule type that signals aggro to another entity.
                        - [ ] ~~Process the aggro signal overlap into `cpu_enemy_awareness.h` somehow.~~
                            - [ ] ~~Something like `double_t last_aggro_signal_time;` and then use the timer to queue up a ready-parry anim?~~
                    - [ ] ~~Enemy heal/consumable -> CPU pinch attack.~~
                        - [ ] 
                    - [ ] ~~Enemy run away -> CPU reapproach attack (can just be pinch attack, or a grab attack, or many ones to pick from).~~
                    - [ ] ~~Enemy close in -> CPU attack.~~
                    - [ ] ~~Enemy do nothing -> CPU 様子見, or CPU attack, or CPU guard.~~
                    - @NOTE: See `Hawsoo_devnotes_enemy_aggression_reading.md` for more info.
                    
                    - [ ] Remove new hitcapsule type and change to an event to broadcast signal that a char is going to attack in 2 frames and they are at `origin` and have `facing_direction` (float/flat/2d) and have `sword_range` distance.
                        - Enemies to that char can determine whether they are in the hitting direction and range, and if they are facing in the direction close to opposite of `facing_direction`, they will put up a ready-parry to oppose the attack. If not, there's nothing they can do.
                        - [x] Removed new hitcapsule type and also the capsule interaction system too.
                        - [x] Convert the "enable hitcapsule group 2" to an event.
                        - [x] Plug in the event into the `cpu_character_enemy_detection.cpp` system, and create a message to get broadcasted to all other `cpu_enemy_awareness.h` components.
                            - See `BT_TRACE("Emplaced attack msg broadcast.");`
                        - [x] ~~Create another system that runs after CPU char enemy detection system.~~ Use `cpu_character_world_sppace_input` to process the broadcasted msgs and try to guard.
                            - Have this one process the broadcasts if any (so that there's no tick lag for this broadcasted message).

                    - [ ] If enemy uses heal/consumable, add attack to attack queue to be executed immediately.
                        - [ ] For now..
                            - [x] Create runtime data event for using consumable.
                            - [x] Broadcast that event in the same way attack is broadcasted.
                            - [x] If enemy, use the broadcasted event to add an attack.
                        - [ ] Once jumptable feature is added..
                            - [ ] @TODO

                    - [ ] If any type of hurt (including parry/guard) occurs, add a random attack to the attack queue.
                        - [x] 一応 (i.e. just put 0 as the attack index)

## (detour) Redo combat AFA and animator to use jumptable system instead of conditional transitions.

- [x] No more conditional transitions.
- [ ] Change AFA to use a string function in its region instead of the row dictating the region.
    - [x] Cut out everything and redo the data structures.
    - [ ] Fix the compilation errors.
        - [x] `update()` fn.
        - [ ] `imgui_renderer.cpp`

- [ ] Use jumptable (higher row has precedence in the check).
    > @NOTE: If there is a transition from the jumptable, no other conditions or mutations will be checked or adhered to in the sim tick.
    - [x] Some planning for how it's gonna work.
    - [ ] jjj.

- [ ] Change player char to use jumptable.
    - [ ] Add movement thing in queue to do a "start-move" and then another for "stop-move".


## (return) Have CPU character attack, and have there be guard, parry, hurt-type interation.

- Stuff that has already been addressed w "Redo combat AFA and animator".
                    - [ ] ~~Create jumptable for transitioning if there are attacks in the attack queue.~~
                        - Include parrying in this too.
                        - 
                        - This will require adding functionality to the AFA system.

- New stuff to work on after "Redo combat AFA and animator".
                    - [ ] If enemy rushes in or enters into a closer range, add close range attack into attack queue to be executed immediately.
                    - [ ] If enemy doesn't do anything back, no attacks are added to attack queue, so it eventually empties.
                        - [ ] When it empties, CPU can jump back and 様子見, or can just 様子見 right in front of their enemy. Whatever is inside the "move" queue I guess.
                            - If the move queue is ever empty, the move queue should just get refilled automatically with random things.
                            - And then occasionally there should be an attack inserted into the attack queue (generating events should happen around when the 様子見 is about over, so that it doesn't feel like it ended ubruptly.)
                    


                - [ ] Make one of each attack type animation.
                    - [ ] Attack.
                    - [ ] Counter attack (more aggressive attack).
                    - [ ] Pinch attack.

        - [ ] Revisit detection: Add line-of-sight detection with raycast. (`@FIXME` tags)
        - [ ] Revisit detection: Ignore suspicious sight cone during SUSPICIOUS state.
            - This is bc I want the CPU to cool-down even if player is in the cone for the SUSPICIOUS state. It's only when entering the detection or AWARE buildup is going up (inside AWARE detection cone) do I feel like the cool-down/calm-down timer should reset to 0.

        - [x] BUGFIX: Detection line when drawn has only the suspicion color.
            - I think that's simply bc even after awareness is checked, suspicion zone is still checked, thus overriding the color from before.
            - FIX: Changes the line drawing so that only the color and whether the line should get drawn only gets mutated once.

- [ ] Get strong attacks implemented (and have strong versions of knockbacks used)
    - Perhaps by having some kind of AFA data point being like "is_strong_attack" that could indicate a strong attack being done?


### Refactor and tech debt stuff.

- [ ] Move all instances of converting `rvec3` to `vec3` into a helper lib (that will eventually change to doing it the "correct" way).
    - (see "// @TODO: Conform to `write_render_transforms.cpp`")

- [ ] Remove @NOCHECKIN and @TEMP tags from commit `570de0f27f0606db66525fb5984e20f391625053` (Faces forward even when moving and integrates strafing in.) (December 8, 2025 at 12:56 AM)
    - It looks like this is going to be quite a lot of refactoring and in general just a lot of work that I'm not ready for.

- [ ] ~~Feature: Time-parametrized state transitions (MAYBE?)~~
    - Transitions out of state after only a certain time?
    - Maybe it could just be accomplished with `can_move`->`is_moving` pairs?
        - This honestly just seems like the most viable way to go about doing this tbh.

- [ ] ~~REFACTOR: Delete the `calc_orig_pt_distance()` method in hitcapsule bc this info is really only needed when doing the actual collision and isn't needed most of the time.~~
    - No. This is used in the spherization of the capsules in the broad phase of the overlap check.

- [ ] DESIGN FIX: Stop having the last frame of root motion be the average of 0th and n-1th frame. Calculate a proper last frame of root motion without interpolation (especially since there are one-off attacks that shouldn't have the beginning of the root motion leaking into the end like that).

- [ ] REFACTOR: Move the AFA data handle from the animator to a component attached to the entity.
    - Bc it seems like everything that needs to use the AFA data handle part of the animator is accessing it from _not_ the renderer, so it should be somewhere else.

- [ ] REFACTOR: Move the animator out into its own component.
    - This just needs to get out, bc reserving a render obj from the renderer and then grabbing the animator from there is just way too much of a hassle.

- [ ] REFACTOR: Allow `Render_object_settings`/`Created_render_object_reference` to have a "mat4 local_transform" property that gets appended to the entity's transform at the end.
    - This allows for no need to have a child object that solely houses the render object (and then needs to rely on the animator and has hectic stuff happen to it bc of that w/ the entity/component relationship)
        - I think this change would make querying for things like animators SOOO much easier, and not require things like `affecting_entity_uuid` to appear in components too.

- [ ] BUGFIX: When selecting an object that has a debug mesh render job, when you switch context from level editor to animation frame action data editor, it crashes bc it can't find the mesh job renderable (dangling pointer).
    - Confirmed that it's when it's displaying a deformed mesh (so if play mode is on and it's player model)
    - [ ] ~~WORKAROUND: Make it so that only can change context when not in play mode?~~
        - Nahh, this wouldn't work. I just need to remove all debug meshes and lines removed when switching contexts.
    - [ ] BONUS POINTS: just remove all debug meshes and lines when in the animator editor context.
