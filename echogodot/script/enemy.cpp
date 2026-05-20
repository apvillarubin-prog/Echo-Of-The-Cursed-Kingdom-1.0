/* Jenova C++ Node Base Script (Smart Enemy AI with Hit Cooldowns) */
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// Config constants
const float AGGRO_RANGE = 200.0f;
const float CHASE_SPEED = 40.0f;
// Removed hard-coded ATTACK_RANGE to allow dynamic sizing

void OnReady(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	if (!self->is_in_group("enemy")) self->add_to_group("enemy");
	self->set_collision_mask_value(2, false);

	self->set_meta("current_health", 15);
	self->set_meta("last_hit_frame", -1);
	self->set_meta("is_dying", false);
	self->set_meta("attack_cooldown", 0.0f); // Prevents animation spamming
	
	UtilityFunctions::print("[HEALTH MONITOR] Enemy '", self->get_name(), "' spawned with 15 isolated HP.");
}

void OnPhysicsProcess(Caller* instance, double delta) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	bool is_dying = self->has_meta("is_dying") ? (bool)self->get_meta("is_dying") : false;
	if (is_dying) return;

	int current_health = self->has_meta("current_health") ? (int)self->get_meta("current_health") : 15;

	// Damage registration
	if (self->has_meta("pending_damage")) {
		int incoming_dmg = (int)self->get_meta("pending_damage");
		self->remove_meta("pending_damage"); 

		current_health -= incoming_dmg;
		self->set_meta("current_health", current_health); 

		UtilityFunctions::print("[HEALTH MONITOR] ", self->get_name(), " registered hit! Damage: ", incoming_dmg, " | HP Left: ", current_health);

		if (current_health <= 0) {
			UtilityFunctions::print("[HEALTH MONITOR] ", self->get_name(), " dead! Removing instance.");
			self->set_meta("is_dying", true);
			self->queue_free();
			return;
		}
	}

	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (!anim) return;

	Vector2 velocity = self->get_velocity();
	Node2D* target = nullptr;

	TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
	if (players.size() > 0) {
		target = Object::cast_to<Node2D>(players[0]);
	}

	// --- Custom Enemy AI Parameters ---
	// Reads unique attack range from Editor Metadata (Defaults to 40.0f if not set)
	float attack_range = self->has_meta("attack_range") ? (float)self->get_meta("attack_range") : 40.0f;
	float attack_cooldown = self->has_meta("attack_cooldown") ? (float)self->get_meta("attack_cooldown") : 0.0f;

	// Process cooldown timer
	if (attack_cooldown > 0.0f) {
		attack_cooldown -= (float)delta;
		self->set_meta("attack_cooldown", attack_cooldown);
	}

	if (target) {
		Vector2 p_pos = target->get_global_position();
		Vector2 e_pos = self->get_global_position();
		float dist = e_pos.distance_to(p_pos);

		if (dist <= AGGRO_RANGE) {
			int dir = (p_pos.x > e_pos.x) ? 1 : -1;
			// Lock facing direction if they are mid-swing
			if (attack_cooldown <= 0.0f || anim->get_animation() != StringName("enemy_attack")) {
				anim->set_flip_h(dir < 0);
			}

			// Chase Phase
			if (dist > attack_range && attack_cooldown <= 0.0f) {
				velocity.x = dir * CHASE_SPEED;
				anim->play("enemy_run");
				self->set_meta("last_hit_frame", -1);
			} 
			// Attack Phase
			else {
				velocity.x = 0; // Stop moving to swing
				
				// Engage Attack Animation
				if (attack_cooldown <= 0.0f) {
					anim->play("enemy_attack");
				}

				if (anim->get_animation() == StringName("enemy_attack")) {
					int current_frame = anim->get_frame();
					int last_hit_frame = self->has_meta("last_hit_frame") ? (int)self->get_meta("last_hit_frame") : -1;

					// Execute damage strictly on animation frame 2
					if (current_frame == 2 && last_hit_frame != 2) {
						float hit_dist = e_pos.distance_to(target->get_global_position());
						
						// Expanded hit registration box to match the new wider attack range
						if (hit_dist <= attack_range + 10.0f) {
							target->call("take_damage", 1);
						}
						self->set_meta("last_hit_frame", 2);
						
						// Set the cooldown timer right after the hit lands (e.g., 1.5 seconds)
						self->set_meta("attack_cooldown", 1.5f);
					} 
					else if (current_frame != 2) {
						self->set_meta("last_hit_frame", -1);
					}
				} 
				// Stand still and idle while waiting for the cooldown timer to finish
				else if (attack_cooldown > 0.0f) {
					anim->play("enemy_idle");
				}
			}
		} else {
			// Out of Aggro Range
			velocity.x = 0;
			anim->play("enemy_idle");
			self->set_meta("last_hit_frame", -1);
		}
	}

	if (!self->is_on_floor()) velocity.y += 1000.0f * (float)delta;
	self->set_velocity(velocity);
	self->move_and_slide();
}

JENOVA_SCRIPT_END
