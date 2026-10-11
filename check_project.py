# @TODO. I want to make a simple that checks for multiple things.
#
# - That only `btglm.h` has #include to cpp_glm header files.
# - (Probably moderately hard but still want it) Top function definition lengths.
# - Stats for the total lines of code (for fun).
# - (MAYBE VERY HARD) All structs in `src/component/` are registered in `component_registry.cpp`.
# - make sure that every vec4, versor, mat2, and mat4 used inside a struct is at its correct alignment (can be an issue on windows w/ AVX since what the compiler expects and what the type forces can be different).
#   - and maybe that the structs themselves also have a compatible alignment w them!
# - make sure that all third party libraries used are the same version number (using git commit hash or conan2?)
