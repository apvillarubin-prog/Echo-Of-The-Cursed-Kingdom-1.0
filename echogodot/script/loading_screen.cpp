/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/classes/audio_stream_player.hpp> // Required for Music
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

		int level = 1;
		
		// Grab the level number from YOUR global script
		Node* my_global = self->get_node_or_null("/root/Global");
		if (my_global && my_global->has_meta("next_level")) {
			level = (int)my_global->get_meta("next_level");
		}

		String scene_path = "res://scene/level" + String::num_int64(level) + ".tscn";
		UtilityFunctions::print("Loading Screen: Going to " + scene_path);
		self->get_tree()->change_scene_to_file(scene_path);
	}
}

JENOVA_SCRIPT_END
