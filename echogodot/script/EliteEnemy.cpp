/* Jenova C++ Node Base Script (Elite Enemy / MiniBoss) */
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
const float CHASE_SPEED = 30.0f;
const float ATTACK_RANGE = 40.0f;
const float ATTACK_COOLDOWN_DURATION = 3.0f;

void OnReady(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	if (!self->is_in_group("enemy")) self->add_to_group("enemy");
	self->set_collision_mask_value(2, false);

	// FIX: Store isolated instance memory values natively
	self->set_meta("current_health", 30);
	self->set_meta("last_hit_frame", -1);
	self->set_meta("is_dying", false);
	self->set_meta("attack_cooldown", 0.0f);
	
	UtilityFunctions::print("[HEALTH MONITOR] MiniBoss '", self->get_name(), "' spawned with 30 isolated HP.");
}

void OnPhysicsProcess(Caller* instance, double delta) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	bool is_dying = self->has_meta("is_dying") ? (bool)self->get_meta("is_dying") : false;
	if (is_dying) return;

	int current_health = self->has_meta("current_health") ? (int)self->get_meta("current_health") : 30;

	// Check notice storage block for fresh damage values
	if (self->has_meta("pending_damage")) {
		int incoming_dmg = (int)self->get_meta("pending_damage");
		self->remove_meta("pending_damage"); 

		current_health -= incoming_dmg;
		self->set_meta("current_health", current_health);

		UtilityFunctions::print("[HEALTH MONITOR] MiniBoss '", self->get_name(), "' registered hit! Damage: ", incoming_dmg, " | Isolated HP Left: ", current_health);

		if (current_health <= 0) {
			UtilityFunctions::print("[HEALTH MONITOR] MiniBoss '", self->get_name(), "' defeated! Purging instance.");
			self->set_meta("is_dying", true);
			self->queue_free();
			return;
		}
	}

	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (!anim) return;

	Vector2 velocity = self->get_velocity();

	// Process localized cooldown transformations
	float attack_cooldown = self->has_meta("attack_cooldown") ? (float)self->get_meta("attack_cooldown") : 0.0f;
	if (attack_cooldown > 0.0f) {
		attack_cooldown -= (float)delta;
		if (attack_cooldown < 0.0f) attack_cooldown = 0.0f;
		self->set_meta("attack_cooldown", attack_cooldown);
	}

	Node2D* target = nullptr;
	TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
	if (players.size() > 0) {
		target = Object::cast_to<Node2D>(players[0]);
	}

	if (target) {
		Vector2 p_pos = target->get_global_position();
		Vector2 e_pos = self->get_global_position();
		float dist = e_pos.distance_to(p_pos);

		if (dist <= AGGRO_RANGE) {
			int dir = (p_pos.x > e_pos.x) ? 1 : -1;
			anim->set_flip_h(dir < 0);

			if (dist > ATTACK_RANGE) {
				velocity.x = dir * CHASE_SPEED;
				anim->play("enemy_run");
				self->set_meta("last_hit_frame", -1);
			} else {
				velocity.x = 0;

				if (attack_cooldown <= 0.0f) {
					anim->play("enemy_attack");

					int current_frame = anim->get_frame();
					int last_hit_frame = self->has_meta("last_hit_frame") ? (int)self->get_meta("last_hit_frame") : -1;

					if (current_frame == 9 && last_hit_frame != 9) {
						float hit_dist = e_pos.distance_to(target->get_global_position());
						if (hit_dist <= 50.0f) {
							target->call("take_damage", 10);
							self->set_meta("attack_cooldown", ATTACK_COOLDOWN_DURATION);
						}
						self->set_meta("last_hit_frame", 2);
					} else if (current_frame != 2) {
						self->set_meta("last_hit_frame", -1);
					}
				} else {
					anim->play("enemy_idle");
					self->set_meta("last_hit_frame", -1);
				}
			}
		} else {
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
