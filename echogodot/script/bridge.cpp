/* Jenova C++ Node Base Script (Puzzle Bridge) */
#include <Godot/godot.hpp>
#include <Godot/classes/static_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/collision_shape2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	StaticBody2D* self = GetSelf<StaticBody2D>(instance);
	if (!self) return;

	if (!self->is_in_group("puzzle_bridge")) self->add_to_group("puzzle_bridge");

	if (!self->has_meta("puzzle_id")) {
		self->set_meta("puzzle_id", 1);
	}

	// Hide the bridge visually at startup
	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (anim) {
		anim->play("invisible"); 
	}
	
	// Ensure collision defaults to disabled so the player falls through until activated
	CollisionShape2D* collision = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
	if (collision) {
		collision->set_disabled(true);
	}
}

void trigger_bridge_appearance(Caller* instance) {
	StaticBody2D* self = GetSelf<StaticBody2D>(instance);
	if (!self) return;

	UtilityFunctions::print("[PUZZLE SYSTEM] Bridge '", self->get_name(), "' received wake signal! Enabling floor collision.");

	// Play the unfold animation frames
	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (anim) {
		anim->play("appear"); 
	}

	// Safely enable the collision box using deferred registration
	CollisionShape2D* collision = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
	if (collision) {
		collision->set_deferred("disabled", false); 
	}
}

JENOVA_SCRIPT_END
