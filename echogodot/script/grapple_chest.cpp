/* Jenova C++ Node Base Script (Grapple Upgrade Chest) */
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

void OnReady(Caller* instance) {
	Area2D* self = GetSelf<Area2D>(instance);
	UtilityFunctions::print("[DEBUG-GRAPPLE] OnReady fired.");
	
	if (self) {
		self->set_meta("has_been_opened", false);
		
		if (self->get_node_or_null("AnimationPlayer")) UtilityFunctions::print("[DEBUG-GRAPPLE] AnimationPlayer found.");
		else UtilityFunctions::print("[ERROR-GRAPPLE] AnimationPlayer NOT found!");
		
		if (self->get_node_or_null("OpenSFX")) UtilityFunctions::print("[DEBUG-GRAPPLE] OpenSFX found.");
		else UtilityFunctions::print("[ERROR-GRAPPLE] OpenSFX NOT found! Check node name.");
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	bool has_been_opened = self->has_meta("has_been_opened") ? (bool)self->get_meta("has_been_opened") : false;
	if (has_been_opened) return;

	bool is_player_near = false;
	Node2D* target_player = nullptr;

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	for (int i = 0; i < bodies.size(); i++) {
		Node* body = Object::cast_to<Node>(bodies[i]);
		if (body && body->is_in_group("player")) {
			is_player_near = true;
			target_player = Object::cast_to<Node2D>(body);
			break;
		}
	}

	Label* prompt = (Label*)self->get_node_or_null("Label");

	if (is_player_near) {
		if (prompt) prompt->set_visible(true);
		
		Input* input = Input::get_singleton();
		if (input->is_action_just_pressed("ui_interact") || input->is_key_pressed(Key::KEY_E)) {
			UtilityFunctions::print("[DEBUG-GRAPPLE] E pressed, opening chest!");
			self->set_meta("has_been_opened", true);
			if (prompt) prompt->set_visible(false);
			
			AnimationPlayer* ap = (AnimationPlayer*)self->get_node_or_null("AnimationPlayer");
			if (ap) ap->play("open");

			AudioStreamPlayer* open_sfx = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("OpenSFX"));
			if (open_sfx) {
				UtilityFunctions::print("[DEBUG-GRAPPLE] Playing OpenSFX...");
				open_sfx->play();
			} else {
				UtilityFunctions::print("[ERROR-GRAPPLE] Tried to play OpenSFX but node is missing!");
			}
			
			if (target_player) {
				UtilityFunctions::print("[DEBUG-GRAPPLE] Calling unlock_grapple on player!");
				target_player->call("unlock_grapple");
			}
		}
	} else {
		if (prompt) prompt->set_visible(false);
	}
}

JENOVA_SCRIPT_END
