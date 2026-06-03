/* Jenova C++ Node Base Script (Disappearing Block) */
#include <Godot/godot.hpp>
#include <Godot/classes/static_body2d.hpp>
#include <Godot/classes/animation_player.hpp>
#include <Godot/classes/collision_shape2d.hpp>
#include <Godot/variant/string_name.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// Define states cleanly
enum BlockState { ASSEMBLED, DISASSEMBLING, DISASSEMBLED, ASSEMBLING };

// Configurable timing configurations (Global read-only configuration constants are fine!)
const float solid_duration = 3.0f;  
const float broken_duration = 2.0f; 

void OnAwake(Caller* instance) {
	// No global assignment! Everything stays local to this execution instance.
}

void OnReady(Caller* instance) {
	StaticBody2D* self = GetSelf<StaticBody2D>(instance);
	if (!self) return;

	AnimationPlayer* anim_player = Object::cast_to<AnimationPlayer>(self->get_node_or_null("AnimationPlayer"));
	CollisionShape2D* collision_shape = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
	
	// Store states directly inside THIS specific node instance's metadata memory space
	self->set_meta("current_state", (int)ASSEMBLED);
	self->set_meta("state_timer", solid_duration);
	
	if (collision_shape) {
		collision_shape->set_deferred("disabled", false);
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	StaticBody2D* self = GetSelf<StaticBody2D>(instance);
	if (!self) return;

	AnimationPlayer* anim_player = Object::cast_to<AnimationPlayer>(self->get_node_or_null("AnimationPlayer"));
	CollisionShape2D* collision_shape = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
	if (!anim_player) return;

	// Retrieve this specific node's state and timer metrics
	BlockState current_state = (BlockState)(int)self->get_meta("current_state", (int)ASSEMBLED);
	float state_timer = (float)self->get_meta("state_timer", solid_duration);

	switch (current_state) {
		case ASSEMBLED:
			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				current_state = DISASSEMBLING;
				anim_player->play("disassemble");
				
				if (collision_shape) {
					collision_shape->set_deferred("disabled", true);
				}
			}
			break;

		case DISASSEMBLING:
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
			if (!anim_player->is_playing()) {
				current_state = ASSEMBLED;
				state_timer = solid_duration;
				
				if (collision_shape) {
					collision_shape->set_deferred("disabled", false);
				}
			}
			break;
	}

	// Save the changes back to this instance's metadata storage
	self->set_meta("current_state", (int)current_state);
	self->set_meta("state_timer", state_timer);
}

JENOVA_SCRIPT_END
