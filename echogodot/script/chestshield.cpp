#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Area2D* chest_self = nullptr;
bool has_been_opened = false;

void OnReady(Caller* instance) {
	chest_self = GetSelf<Area2D>(instance);
	if (chest_self) {
		UtilityFunctions::print("DEBUG [ShieldChest] OnReady fired.");
		AnimationPlayer* ap = (AnimationPlayer*)chest_self->get_node_or_null("AnimationPlayer");
		if (ap) UtilityFunctions::print("DEBUG [ShieldChest] AnimationPlayer found.");
		else UtilityFunctions::print("DEBUG [ShieldChest] AnimationPlayer NOT found!");
		Label* prompt = (Label*)chest_self->get_node_or_null("Label");
		if (prompt) UtilityFunctions::print("DEBUG [ShieldChest] Label found.");
		else UtilityFunctions::print("DEBUG [ShieldChest] Label NOT found!");
	} else {
		UtilityFunctions::print("DEBUG [ShieldChest] OnReady — self is NULL!");
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
		if (Input::get_singleton()->is_key_pressed(Key::KEY_E)) {
			UtilityFunctions::print("DEBUG [ShieldChest] E pressed, opening chest.");
			has_been_opened = true;
			if (prompt) prompt->set_visible(false);
			AnimationPlayer* ap = (AnimationPlayer*)chest_self->get_node_or_null("AnimationPlayer");
			if (ap) ap->play("open");
			// calls unlock_shield on the player instead of unlock_sword
			if (target_player) target_player->call("unlock_shield");
		}
	} else {
		if (prompt) prompt->set_visible(false);
	}
}

JENOVA_SCRIPT_END
