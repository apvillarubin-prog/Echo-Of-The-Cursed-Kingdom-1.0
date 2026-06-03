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

	// First, try assuming the CanvasLayer is a sibling in the Level tree
	Node* ui_node = self->get_node_or_null("../CanvasLayer/WinUI");
	
	// If it wasn't found there, try assuming it's a direct child of the Door
	if (!ui_node) {
		ui_node = self->get_node_or_null("CanvasLayer/WinUI");
	}

	if (ui_node)
	{
		win_ui = Object::cast_to<Control>(ui_node);
		if (win_ui) win_ui->set_visible(false); // Hide the UI initially

		// Safely cast buttons using their nested container paths
		Button* yes_btn = Object::cast_to<Button>(win_ui->get_node_or_null("CenterContainer/VBoxContainer/HBoxContainer/YesButton"));
		Button* no_btn = Object::cast_to<Button>(win_ui->get_node_or_null("CenterContainer/VBoxContainer/HBoxContainer/NoButton"));

		if (yes_btn) yes_btn->connect("pressed", Callable(self, "on_yes_pressed"));
		if (no_btn) no_btn->connect("pressed", Callable(self, "on_no_pressed"));
		
		UtilityFunctions::print("[DEBUG] WinUI successfully found and buttons connected!");
	}
	else
	{
		UtilityFunctions::print("[ERROR] WinUI not found! Double check your Node Path in the Scene dock.");
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
			else
			{
				UtilityFunctions::print("[ERROR] WinUI is null. Cannot make it visible.");
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
	if (!self) return;
	
	// 1. Get the actual file path of the scene (e.g., "res://scene/level1.tscn")
	String scene_path = self->get_tree()->get_current_scene()->get_scene_file_path();
	
	// 2. Extract just the file name without extension (e.g., "level1")
	String file_name = scene_path.get_file().get_basename().to_lower();
	
	// 3. Strip out the word "level" to isolate the number
	String num_str = file_name.replace("level", "");
	
	int current_level = 1; // Default
	if (num_str.is_valid_int()) {
		current_level = num_str.to_int();
	} else if (Engine::get_singleton()->has_meta("next_level")) {
		// Fallback just in case
		current_level = (int)Engine::get_singleton()->get_meta("next_level") - 1;
	}
	
	// 4. Calculate the next level
	int next_level_to_load = current_level + 1;
	
	// 5. Save the new level to the Engine meta so the loading screen can grab it
	Engine::get_singleton()->set_meta("next_level", next_level_to_load);
	
	UtilityFunctions::print("Proceeding to Level ", next_level_to_load, "...");
	
	// Transition to the loading screen
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}

JENOVA_SCRIPT_END
