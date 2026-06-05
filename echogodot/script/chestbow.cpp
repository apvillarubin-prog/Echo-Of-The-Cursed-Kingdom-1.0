/* Jenova C++ Node Base Script (Bow Chest) */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/classes/audio_stream_player.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Area2D* chest_self = nullptr;
bool has_been_opened = false;

void OnReady(Caller* instance) {
	chest_self = GetSelf<Area2D>(instance);
	UtilityFunctions::print("[DEBUG-BOW] OnReady fired.");
	
	if (chest_self) {
		if (chest_self->get_node_or_null("AnimationPlayer")) UtilityFunctions::print("[DEBUG-BOW] AnimationPlayer found.");
		else UtilityFunctions::print("[ERROR-BOW] AnimationPlayer NOT found!");
		
		if (chest_self->get_node_or_null("OpenSFX")) UtilityFunctions::print("[DEBUG-BOW] OpenSFX found.");
		else UtilityFunctions::print("[ERROR-BOW] OpenSFX NOT found! Check node name.");
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!chest_self || has_been_opened) return;

	bool is_player_near = false;
	Node2D* target_player = nullptr;

	TypedArray<Node2D> bodies = chest_self->get_overlapping_bodies();
	for (int i = 0; i < bodies.size(); i++) {
		Node* body = Object::cast_to<Node>(bodies[i]);
		if (body && body->is_in_group("player")) {
			is_player_near = true;
			target_player = Object::cast_to<Node2D>(body);
			break;
		}
	}

	Label* prompt = (Label*)chest_self->get_node_or_null("Label");

	if (is_player_near) {
		if (prompt) prompt->set_visible(true);
		
		if (Input::get_singleton()->is_action_just_pressed("ui_interact") || Input::get_singleton()->is_key_pressed(Key::KEY_E)) {
			UtilityFunctions::print("[DEBUG-BOW] E pressed, opening chest!");
			has_been_opened = true;
			if (prompt) prompt->set_visible(false);
			
			AnimationPlayer* ap = (AnimationPlayer*)chest_self->get_node_or_null("AnimationPlayer");
			if (ap) ap->play("open");
			
			AudioStreamPlayer* open_sfx = Object::cast_to<AudioStreamPlayer>(chest_self->get_node_or_null("OpenSFX"));
			if (open_sfx) {
				UtilityFunctions::print("[DEBUG-BOW] Playing OpenSFX...");
				open_sfx->play();
			} else {
				UtilityFunctions::print("[ERROR-BOW] Tried to play OpenSFX but node is missing!");
			}
			
			if (target_player) {
				UtilityFunctions::print("[DEBUG-BOW] Calling unlock_bow on player!");
				target_player->call("unlock_bow");
			}
		}
	} else {
		if (prompt) prompt->set_visible(false);
	}
}

JENOVA_SCRIPT_END
