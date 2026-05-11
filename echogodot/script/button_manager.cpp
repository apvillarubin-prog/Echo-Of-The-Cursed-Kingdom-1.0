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
	
	
	Button* start_btn = self->get_node<Button>("start");
	Button* option_btn = self->get_node<Button>("option");
	Button* menu_btn = self->get_node<Button>("menu");

	
	if (start_btn) {
		start_btn->connect("pressed", Callable(self, "on_start_pressed"));
	}

	
	if (menu_btn) {
		menu_btn->connect("pressed", Callable(self, "on_quit_pressed"));
	}
}


void on_start_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	UtilityFunctions::print("Menu: Switching to Loading Screen...");
	
	
	self->get_tree()->change_scene_to_file("res://scene/levels.tscn");
}


void on_quit_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	UtilityFunctions::print("Menu: Quitting Game...");
	self->get_tree()->quit();
}

JENOVA_SCRIPT_END
