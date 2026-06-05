#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/button.hpp> // ADDED: Required for Button nodes
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/window.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Control* self = nullptr;
Label* credits_label = nullptr;
Button* back_button = nullptr;


float scroll_speed = 65.0f;       
float dynamic_end_y = 0.0f;       
bool initialized = false;

void OnAwake(Caller* instance) {
	self = GetSelf<Control>(instance);
	if (self) {
		credits_label = Object::cast_to<Label>(self->get_node_or_null("Label"));
		back_button = Object::cast_to<Button>(self->get_node_or_null("Button"));
	}
}

void OnReady(Caller* instance) {
	if (credits_label) {
		float screen_height = 720.0f; 
		Window* window = credits_label->get_window();
		if (window) {
			screen_height = (float)window->get_size().y;
		}
		Vector2 starting_pos = credits_label->get_position();
		starting_pos.y = screen_height + 50.0f; 
		credits_label->set_position(starting_pos);

		dynamic_end_y = -(credits_label->get_size().y);
		
		initialized = true;
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!credits_label || !initialized) return;


	Vector2 current_pos = credits_label->get_position();
	current_pos.y -= scroll_speed * (float)delta;
	credits_label->set_position(current_pos);

	if (current_pos.y < dynamic_end_y) {
		if (self && self->get_tree()) {
			self->get_tree()->change_scene_to_file("res://mainmenu.tscn");
		}
		initialized = false; 
	}
}

void _on_back_button_pressed() {
	if (self && self->get_tree()) {
		self->get_tree()->change_scene_to_file("res://mainmenu.tscn");
	}
}

JENOVA_SCRIPT_END
