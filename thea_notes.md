- [x] Get the physics_engine folder to not have any .txt files (make all the .cpp files compile correctly)
    - It doesn't seem toooo difficult, but there needs to be some work on getting this to use TXP_renderer instead.

- [x] Integrate the TXP renderer.

- [x] Add in deformed render models being disabled while `is_simulation_running` is false.

- [x] Fix asserts in creating physics objects.
    - It appears to be workign.

- [x] Fix input controlled character mvt (system).
    - Ummm that was ez. Like super duper ez.

- [x] do fixing up until afa required stuff.

- [x] make orbiting cam mode for input stuff.

- [x] do animated render models implementation in txp-renderer.
- [x] include afa implementation.

- [x] Implement the new txp-renderer into here.

- [ ] Reimplement btafa processing here.

- [x] Add a component type that allows for string of chars to signify rail system.

- [x] Create list of transforms imgui list/window.
    - [x] For some reaosn it's crashing w imguizmo?
        - possibly wrong version?
        - after updating the imguizmo version, it's still crashing... hmmm.
        - maybe setup is missing for imguizmo specifically???
    - [x] also show root objects that have component::Transform but no component::Trasfnorm_hierarchy component.
    - [x] Now imguizmo is just plain not showing up????
        - The draw context is 0x0 size for some reason???
        - I needed to do setRect()
        - Also, to have the correct state i needed to actually use matching versions of imgui and imguizmo.
    - [x] Insert imguizmo into the actual correct windows.
        - probably as some kind of callback or a list of requests maybe??
            - like giving a transform and then requesting the information back if it changed.
            - this could allow for many different gizmos to be drawn per view for bezier curves or smth. plus, no need to include the imguizmo headers in the game engine ig??
        - [x] remove imguizmo dependency.
    - [x] Fix stuff not moving for transforms without a transform hierarchy from imguizmo move!!
- [x] Create a simple imgui inspector for the component.
- [x] fix rename todo.

- [x] Have component draw multiple models of the rails in their positions.

- [x] Implement tilt rails to go into and out of curves.

- [ ] Make component for riding rail lines (essentially having a whole ass train car system)

- [ ] Fix construction code change not causing the rail line to change. or is it just not deleting old entities?
    - maybe need to use the create_entity() and destroy_entity() funcs in the entity container?????
    - turns out the created entities were never listed. but... now there's hidden entities that wont show up in the object list. which for the rail line is probably fine but hey idk.
        - [ ] make it explicit w comment or something that the creation method is meant to not show the rails on the object list.
    - [ ] fix the renderer crashing code when the rails are rebuilt.