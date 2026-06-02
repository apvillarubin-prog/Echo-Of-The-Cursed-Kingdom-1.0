#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/sprite_frames.hpp> 
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/progress_bar.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

const float AGGRO_RANGE = 170.0f;
const float CHASE_SPEED = 30.0f;
const float ATTACK_RANGE = 35.0f; 
const float ATTACK_COOLDOWN_DURATION = 1.5f; 

void OnReady(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	if (!self->is_in_group("enemy")) self->add_to_group("enemy");
	self->set_collision_mask_value(2, false);

	self->set_meta("current_health", 60);
	self->set_meta("attack_cooldown", 0.0f);
	self->set_meta("consecutive_attacks", 0); 
	
	ProgressBar* hp_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("HealthBar"));
	if (hp_bar) {
		hp_bar->set_max(60.0); 
		hp_bar->set_value(60.0);
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;
	if (self->has_meta("is_dying") && (bool)self->get_meta("is_dying")) return;

	int current_health = self->has_meta("current_health") ? (int)self->get_meta("current_health") : 60;

	if (self->has_meta("pending_damage")) {
		int incoming_dmg = (int)self->get_meta("pending_damage");
		self->remove_meta("pending_damage"); 
		current_health -= incoming_dmg;
		self->set_meta("current_health", current_health);

		ProgressBar* hp_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("HealthBar"));
		if (hp_bar) hp_bar->set_value((double)current_health);

		if (current_health <= 0) {
			self->set_meta("is_dying", true);
			self->queue_free();
			return;
		}
	}

	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (!anim) return;

	Vector2 velocity = self->get_velocity();

	float attack_cooldown = self->has_meta("attack_cooldown") ? (float)self->get_meta("attack_cooldown") : 0.0f;
	if (attack_cooldown > 0.0f) {
		attack_cooldown -= (float)delta;
		self->set_meta("attack_cooldown", attack_cooldown);
	}

	Node2D* target = nullptr;
	TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
	if (players.size() > 0) target = Object::cast_to<Node2D>(players[0]);

	if (target) {
		Vector2 p_pos = target->get_global_position();
		Vector2 e_pos = self->get_global_position();
		float dist = e_pos.distance_to(p_pos);

		if (dist <= AGGRO_RANGE) {
			int dir = (p_pos.x > e_pos.x) ? 1 : -1;
			
			bool is_playing_attack = (anim->get_animation() == StringName("enemy_attack") && anim->is_playing());
			if (!is_playing_attack) anim->set_flip_h(dir < 0);

			if (dist > ATTACK_RANGE && !is_playing_attack) {
				if (attack_cooldown <= 0.0f) {
					velocity.x = dir * CHASE_SPEED;
					if (anim->get_animation() != StringName("enemy_run")) anim->play("enemy_run");
				} else {
					velocity.x = 0;
					if (anim->get_animation() != StringName("enemy_idle")) anim->play("enemy_idle");
				}
				self->set_meta("last_hit_frame", -1);
			} 
			else {
				velocity.x = 0; 

				if (attack_cooldown <= 0.0f && !is_playing_attack) {
					anim->play("enemy_attack");
					anim->set_frame(0); 
					self->set_meta("last_hit_frame", -1);
				}

				if (is_playing_attack) {
					int current_frame = anim->get_frame();
					int last_hit_frame = self->has_meta("last_hit_frame") ? (int)self->get_meta("last_hit_frame") : -1;

					if (current_frame == 3 && last_hit_frame != 3) {
						bool was_parried = false;
						
						if (e_pos.distance_to(target->get_global_position()) <= 50.0f) {
							// Call the player's damage function and check if they parried it!
							Variant result = target->call("take_damage", 10);
							if (result.get_type() == Variant::Type::BOOL && (bool)result) {
								was_parried = true;
							}
						}
						
						self->set_meta("last_hit_frame", 3);

						// --- STUN LOGIC ---
						if (was_parried) {
							UtilityFunctions::print("[DEBUG] Elite Parried! STUNNED for 3 seconds!");
							self->set_meta("attack_cooldown", 3.0f); // Massive Stun
							self->set_meta("consecutive_attacks", 0); // Cancel the double-swing
							anim->play("enemy_idle"); // Force stagger animation
							return; // Skip the rest of the physics frame
						}
					}
					
					int max_frames = anim->get_sprite_frames()->get_frame_count("enemy_attack");
					if (current_frame == max_frames - 1) {
						int consecutive = self->has_meta("consecutive_attacks") ? (int)self->get_meta("consecutive_attacks") : 0;
						consecutive++;
						
						if (consecutive >= 2) {
							self->set_meta("attack_cooldown", ATTACK_COOLDOWN_DURATION); 
							consecutive = 0; 
						} else {
							self->set_meta("attack_cooldown", 0.0f); 
						}
						
						self->set_meta("consecutive_attacks", consecutive);
						anim->stop(); 
					}
				} 
				else if (attack_cooldown > 0.0f) {
					if (anim->get_animation() != StringName("enemy_idle")) anim->play("enemy_idle");
				}
			}
		} else {
			velocity.x = 0;
			if (anim->get_animation() != StringName("enemy_idle")) anim->play("enemy_idle");
			self->set_meta("last_hit_frame", -1);
		}
	}

	if (!self->is_on_floor()) velocity.y += 1000.0f * (float)delta;
	self->set_velocity(velocity);
	self->move_and_slide();
}

JENOVA_SCRIPT_END
