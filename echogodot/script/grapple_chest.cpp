/* Jenova C++ Node Base Script (Grapple Upgrade Chest) */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (self) {
		// Store open status natively on this specific chest node instance
		self->set_meta("has_been_opened", false);
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Read instance-specific meta flag to ensure multiple chests don't conflict
	bool has_been_opened = self->has_meta("has_been_opened") ? (bool)self->get_meta("has_been_opened") : false;
	if (has_been_opened) return;

	bool is_player_near = false;
	Node2D* target_player = nullptr;

	// Scan overlapping bodies for our player node
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
		// Listens for your customized E key binding map layout
		if (input->is_action_just_pressed("ui_interact") || input->is_key_pressed(Key::KEY_E)) {
			self->set_meta("has_been_opened", true);
			if (prompt) prompt->set_visible(false);
			
			AnimationPlayer* ap = (AnimationPlayer*)self->get_node_or_null("AnimationPlayer");
			if (ap) ap->play("open");
			
			// Dispatches the unlock_grapple command to the player master script completely separate from the bow!
			if (target_player) {
				target_player->call("unlock_grapple");
				UtilityFunctions::print("[CHEST MONITOR] Metroidvania Progression Triggered: Grappling hook mechanism active!");
			}
		}
	} else {
		if (prompt) prompt->set_visible(false);
	}
}

JENOVA_SCRIPT_END
