/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);

	// Get references to your buttons based on your screenshot
	Button* lvl1_btn = self->get_node<Button>("Button");
	Button* lvl2_btn = self->get_node<Button>("Button2");
	Button* lvl3_btn = self->get_node<Button>("Button3");

	// Connect them to their functions
	if (lvl1_btn) lvl1_btn->connect("pressed", Callable(self, "on_lvl1_pressed"));
	if (lvl2_btn) lvl2_btn->connect("pressed", Callable(self, "on_lvl2_pressed"));
	if (lvl3_btn) lvl3_btn->connect("pressed", Callable(self, "on_lvl3_pressed"));
}

void on_lvl1_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	UtilityFunctions::print("Level 1 Selected!");
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

void on_lvl2_pressed(Caller* instance)
{
	UtilityFunctions::print("Level 2 is currently locked!");
}

void on_lvl3_pressed(Caller* instance)
{
	UtilityFunctions::print("Level 3 is currently locked!");
}

JENOVA_SCRIPT_END
