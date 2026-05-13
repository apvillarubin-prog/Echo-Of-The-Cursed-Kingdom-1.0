/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/variant/utility_functions.hpp>
using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Button* lvl1_btn = self->get_node<Button>("Button");
	Button* lvl2_btn = self->get_node<Button>("Button2");
	Button* lvl3_btn = self->get_node<Button>("Button3");
	Button* lvl4_btn = self->get_node<Button>("Button4"); // ADD THIS

	if (lvl1_btn) lvl1_btn->connect("pressed", Callable(self, "on_lvl1_pressed"));
	if (lvl2_btn) lvl2_btn->connect("pressed", Callable(self, "on_lvl2_pressed"));
	if (lvl3_btn) lvl3_btn->connect("pressed", Callable(self, "on_lvl3_pressed"));
	if (lvl4_btn) lvl4_btn->connect("pressed", Callable(self, "on_lvl4_pressed")); // ADD THIS
}

void on_lvl1_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Engine::get_singleton()->set_meta("next_level", 1);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

void on_lvl2_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Engine::get_singleton()->set_meta("next_level", 2);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

void on_lvl3_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Engine::get_singleton()->set_meta("next_level", 3);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

void on_lvl4_pressed(Caller* instance) // ADD THIS
{
	Node* self = GetSelf<Node>(instance);
	Engine::get_singleton()->set_meta("next_level", 4);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

JENOVA_SCRIPT_END
