#include <Godot/godot.hpp>
#include <Godot/classes/static_body2d.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/collision_shape2d.hpp>
#include <Godot/variant/string_name.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

StaticBody2D* self = nullptr;
AnimationPlayer* anim_player = nullptr;
CollisionShape2D* collision_shape = nullptr;

// --- State Machine Definitions ---
enum BlockState { ASSEMBLED, DISASSEMBLING, DISASSEMBLED, ASSEMBLING };
BlockState current_state = ASSEMBLED;

// Configurable timing configurations
float solid_duration = 3.0f;  // How long the block remains safe to stand on
float broken_duration = 2.0f; // How long it stays completely vanished
float state_timer = 0.0f;

void OnAwake(Caller* instance) {
	self = GetSelf<StaticBody2D>(instance);
}

void OnReady(Caller* instance) {
	if (self) {
		anim_player = Object::cast_to<AnimationPlayer>(self->get_node_or_null("AnimationPlayer"));
		collision_shape = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
		
		// Reset state on load
		current_state = ASSEMBLED;
		state_timer = solid_duration;
		
		if (collision_shape) {
			collision_shape->set_deferred("disabled", false);
		}
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || !anim_player) return;

	switch (current_state) {
		case ASSEMBLED:
			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				current_state = DISASSEMBLING;
				anim_player->play("disassemble");
				
				// CRITICAL: Turn off collisions instantly the moment it shatters
				if (collision_shape) {
					collision_shape->set_deferred("disabled", true);
				}
			}
			break;

		case DISASSEMBLING:
			// Automatically wait until the destruction animation finishes playing
			if (!anim_player->is_playing()) {
				current_state = DISASSEMBLED;
				state_timer = broken_duration;
			}
			break;

		case DISASSEMBLED:
			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				current_state = ASSEMBLING;
				anim_player->play("assemble");
			}
			break;

		case ASSEMBLING:
			// Wait until the blocks piece themselves completely back together
			if (!anim_player->is_playing()) {
				current_state = ASSEMBLED;
				state_timer = solid_duration;
				
				// CRITICAL: Reactivate solid collision code exactly when the visual is whole
				if (collision_shape) {
					collision_shape->set_deferred("disabled", false);
				}
			}
			break;
	}
}

JENOVA_SCRIPT_END
