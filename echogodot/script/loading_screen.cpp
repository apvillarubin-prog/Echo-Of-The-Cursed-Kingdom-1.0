/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/scene_tree_timer.hpp> // Critical for create_timer
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	if (!self) return;

	UtilityFunctions::print("Loading Screen: Initialized.");


	self->get_tree()->create_timer(1.5)->connect("timeout", Callable(self, "on_timeout"));
}

// This function is triggered once the 1.5 seconds are up
void on_timeout(Caller* instance)
{
	Control* self = GetSelf<Control>(instance);
	if (!self) return;

	UtilityFunctions::print("Loading Screen: Transitioning to Level 1...");
	
	/* CRITICAL: Replace the path below with your ACTUAL level file path.
	   Right-click your level scene in Gox`dot's FileSystem and select 'Copy Path'.
	*/
	self->get_tree()->change_scene_to_file("res://scene/level1.tscn");
}

JENOVA_SCRIPT_END
