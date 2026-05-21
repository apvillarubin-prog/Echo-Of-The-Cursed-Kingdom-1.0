/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/variant/utility_functions.hpp>
using namespace godot;
using namespace jenova::sdk;

static double elapsed = 0.0;
static bool switched = false;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	if (!self) return;
	elapsed = 0.0;
	switched = false;
	UtilityFunctions::print("Loading Screen: Initialized.");
}

void OnProcess(Caller* instance, double delta)
{
	Control* self = GetSelf<Control>(instance);
	if (!self || switched) return;

	elapsed += delta;
	if (elapsed >= 3.5) // Holds the screen for 3.5 seconds to showcase the animation
	{
		switched = true;
		int level = 1;
		if (Engine::get_singleton()->has_meta("next_level"))
			level = (int)Engine::get_singleton()->get_meta("next_level");

		String scene_path = "res://scene/level" + String::num_int64(level) + ".tscn";
		UtilityFunctions::print("Loading Screen: Going to " + scene_path);
		self->get_tree()->change_scene_to_file(scene_path);
	}
}

JENOVA_SCRIPT_END
