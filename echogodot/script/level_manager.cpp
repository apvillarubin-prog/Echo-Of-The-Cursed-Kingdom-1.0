/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/variant/utility_functions.hpp>
using namespace godot;
using namespace jenova::sdk;
JENOVA_SCRIPT_BEGIN
void OnReady(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);

	// --- MUSIC MANAGEMENT ---
	AudioStreamPlayer* menu_music = nullptr;
	Node* music_node = self->get_node_or_null("/root/MusicManager");
	if (music_node) {
		menu_music = Object::cast_to<AudioStreamPlayer>(music_node);
		if (menu_music && !menu_music->is_playing()) {
			menu_music->play();
		}
	}
	// ------------------------

	// Safely grab and cast all the buttons
	Button* lvl1_btn = Object::cast_to<Button>(self->get_node_or_null("Button"));
	Button* lvl2_btn = Object::cast_to<Button>(self->get_node_or_null("Button2"));
	Button* lvl3_btn = Object::cast_to<Button>(self->get_node_or_null("Button3"));
	Button* lvl4_btn = Object::cast_to<Button>(self->get_node_or_null("Button4"));
	if (lvl1_btn) lvl1_btn->connect("pressed", Callable(self, "on_lvl1_pressed"));
	if (lvl2_btn) lvl2_btn->connect("pressed", Callable(self, "on_lvl2_pressed"));
	if (lvl3_btn) lvl3_btn->connect("pressed", Callable(self, "on_lvl3_pressed"));
	if (lvl4_btn) lvl4_btn->connect("pressed", Callable(self, "on_lvl4_pressed"));
}
void on_lvl1_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Node* my_global = self->get_node_or_null("/root/Global");
	if (my_global) my_global->set_meta("next_level", 1);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}
void on_lvl2_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Node* my_global = self->get_node_or_null("/root/Global");
	if (my_global) my_global->set_meta("next_level", 2);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}
void on_lvl3_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Node* my_global = self->get_node_or_null("/root/Global");
	if (my_global) my_global->set_meta("next_level", 3);
	self->get_tree()->change_scene_to_file("res://assetLvl3/state1.tscn");
}
void on_lvl4_pressed(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	Node* my_global = self->get_node_or_null("/root/Global");
	if (my_global) my_global->set_meta("next_level", 4);
	self->get_tree()->change_scene_to_file("res://scene/loading_screen.tscn");
}
JENOVA_SCRIPT_END
