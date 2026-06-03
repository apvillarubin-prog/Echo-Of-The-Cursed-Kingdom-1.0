/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/window.hpp>           // FIX 1: Resolves the undefined godot::Window error
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/callable.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// Forward declarations
void on_start_pressed(Caller* instance);
void on_credits_pressed(Caller* instance);
void on_quit_pressed(Caller* instance);

void OnReady(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	if (!self) return;

	// Safety check for MusicManager via root window
	Window* root = self->get_tree()->get_root();
	if (root) {
		Node* music_node = root->get_node_or_null("MusicManager");
		AudioStreamPlayer* menu_music = Object::cast_to<AudioStreamPlayer>(music_node);
		if (menu_music && !menu_music->is_playing()) {
			menu_music->play();
		}
	}

	// FIX 2: Safe casting using Object::cast_to to fix the C2275 template compiler errors
	Button* start_btn  = Object::cast_to<Button>(self->get_node_or_null("MenuButtons/start"));
	Button* credits_btn = Object::cast_to<Button>(self->get_node_or_null("MenuButtons/credits"));
	Button* quit_btn   = Object::cast_to<Button>(self->get_node_or_null("MenuButtons/quit"));

	if (start_btn) {
		start_btn->connect("pressed", Callable(self, "on_start_pressed"));
	}
	if (credits_btn) {
		credits_btn->connect("pressed", Callable(self, "on_credits_pressed"));
	}
	if (quit_btn) {
		quit_btn->connect("pressed", Callable(self, "on_quit_pressed"));
	}
}

void on_start_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	if (self && self->get_tree()) {
		self->get_tree()->change_scene_to_file("res://scene/levels.tscn");
	}
}

void on_credits_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	if (self && self->get_tree()) {
		self->get_tree()->change_scene_to_file("res://scene/credits.tscn");
	}
}

void on_quit_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	if (self && self->get_tree()) {
		self->get_tree()->quit();
	}
}

JENOVA_SCRIPT_END
