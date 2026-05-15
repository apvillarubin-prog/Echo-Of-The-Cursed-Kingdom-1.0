/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Control* win_ui = nullptr;

void OnReady(Caller* instance)
{
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Connect the body_entered signal to detect the player
	self->connect("body_entered", Callable(self, "on_body_entered"));

	// Grab the UI node through the CanvasLayer
	Node* ui_node = self->get_node_or_null("CanvasLayer/WinUI");
	if (ui_node)
	{
		win_ui = Object::cast_to<Control>(ui_node);
		if (win_ui) win_ui->set_visible(false); // Hide the UI initially

		// Safely cast buttons using their nested container paths
		Button* yes_btn = Object::cast_to<Button>(win_ui->get_node_or_null("CenterContainer/VBoxContainer/HBoxContainer/YesButton"));
		Button* no_btn = Object::cast_to<Button>(win_ui->get_node_or_null("CenterContainer/VBoxContainer/HBoxContainer/NoButton"));

		if (yes_btn) yes_btn->connect("pressed", Callable(self, "on_yes_pressed"));
		if (no_btn) no_btn->connect("pressed", Callable(self, "on_no_pressed"));
	}
}

// Triggered when something touches the invisible door collision
void on_body_entered(Caller* instance, Node2D* body)
{
	if (body && body->is_in_group("player"))
	{
		// Ask the player how many items they have
		Variant inv_variant = body->call("get_inventory_count");
		int current_items = (int)inv_variant;

		if (current_items >= 3)
		{
			UtilityFunctions::print("Player has all 3 items! You win!");
			if (win_ui)
			{
				win_ui->set_visible(true); // Show the "You Win" prompt
			}
		}
		else
		{
			UtilityFunctions::print("Door locked! You need 3 items. You only have: ", current_items);
		}
	}
}

// Triggered when the player clicks "Yes"
void on_yes_pressed(Caller* instance)
{
	Area2D* self = GetSelf<Area2D>(instance);
	
	// Set the next level meta to 2
	Engine::get_singleton()->set_meta("next_level", 2);
	
	UtilityFunctions::print("Proceeding to Level 2...");
	
	// Transition to the loading screen
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

// Triggered when the player clicks "No"
void on_no_pressed(Caller* instance)
{
	// Hide the UI so the player can keep walking around Level 1
	if (win_ui)
	{
		win_ui->set_visible(false);
	}
}

JENOVA_SCRIPT_END
