/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/audio_stream_player.hpp>
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
	if (elapsed >= 3.5)
	{
		switched = true;

		// --- MUSIC MANAGEMENT ---
		Node* music_node = self->get_node_or_null("/root/MusicManager");
		if (music_node) {
			AudioStreamPlayer* menu_music = Object::cast_to<AudioStreamPlayer>(music_node);
			if (menu_music && menu_music->is_playing()) {
				menu_music->stop();
			}
		}
		// ------------------------

		int level = 1; // Default

		// Read from the Global Autoload
		Node* my_global = self->get_node_or_null("/root/Global");
		UtilityFunctions::print("[DEBUG] Searching for /root/Global...");
		if (my_global) {
			UtilityFunctions::print("[DEBUG] /root/Global FOUND.");
			if (my_global->has_meta("next_level")) {
				Variant meta_val = my_global->get_meta("next_level");
				level = (int)meta_val;
				UtilityFunctions::print("[DEBUG] next_level meta found. Value: ", level);
			} else {
				UtilityFunctions::print("[ERROR] /root/Global exists but has NO next_level meta! Defaulting to level 1.");
			}
		} else {
			UtilityFunctions::print("[ERROR] /root/Global NOT FOUND. Defaulting to level 1.");
		}

		UtilityFunctions::print("[DEBUG] Resolved level int: ", level);

		// --- SCENE PATH LOOKUP TABLE ---
		String scene_path;
		switch (level)
		{
			case 3:
				scene_path = "res://assetLvl3/state1.tscn";
				break;
			default:
				scene_path = "res://scene/level" + String::num_int64(level) + ".tscn";
				break;
		}
		// --------------------------------

		UtilityFunctions::print("[DEBUG] Final scene_path resolved to: ", scene_path);
		UtilityFunctions::print("[DEBUG] Attempting scene change now...");
		self->get_tree()->change_scene_to_file(scene_path);
		UtilityFunctions::print("[DEBUG] change_scene_to_file() called. (This may not print if scene change is immediate.)");
	}
}
JENOVA_SCRIPT_END
