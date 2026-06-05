#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/sprite_frames.hpp>
#include <Godot/classes/ray_cast2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/color_rect.hpp>
#include <Godot/classes/progress_bar.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <Godot/classes/kinematic_collision2d.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/string_name.hpp>
#include <Godot/variant/color.hpp>
#include <cmath>
#include <algorithm>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
RayCast2D* ledge_check = nullptr;
ColorRect* smash_indicator = nullptr; 
ProgressBar* hp_bar = nullptr; 

// --- Sound Effects Nodes ---
AudioStreamPlayer2D* hit_sound = nullptr; 
AudioStreamPlayer2D* windup_sound = nullptr; 
AudioStreamPlayer2D* smash_sound = nullptr;  
AudioStreamPlayer2D* death_sound = nullptr;  

enum OrcState { 
	STATE_PATROL, 
	STATE_CHASE, 
	STATE_ATTACK_NORMAL,
	STATE_SMASH_LAUNCH, 
	STATE_SMASH_AIR, 
	STATE_SMASH_LAND, 
	STATE_DEATH
};
OrcState current_state = STATE_PATROL;

// --- Orc Stats ---
const int MAX_HEALTH = 160;
int health = MAX_HEALTH;          
bool is_dead = false;      
int smash_damage = 22;
float default_gravity = 980.0f;

// BALANCING FIX: Reduced radius to 75.0f so the box is 150px wide (perfect for dodging)
float smash_splash_radius = 75.0f; 

// --- Momentum & Logic ---
float walk_speed = 45.0f;
float run_speed = 90.0f;
float smash_horizontal_speed = 220.0f;
float ground_acceleration = 400.0f; 
float ground_friction = 600.0f;
float fixed_floor_y = 80.0f;   

// --- Timers & Internal Tracking ---
float state_timer = 0.0f;
float smash_cooldown = 0.0f; 
int direction = -1;
float target_smash_x = 0.0f;
bool is_dive_bombing = false;
Node2D* last_ignored_player = nullptr; 

void safely_clear_exception() {
	if (self && last_ignored_player) {
		self->remove_collision_exception_with(last_ignored_player);
		last_ignored_player = nullptr;
	}
}

void update_hp_bar() {
	if (hp_bar) {
		hp_bar->set_value(health);
		hp_bar->set_visible(health < MAX_HEALTH && health > 0);
	}
}

void play_hit_effects() {
	if (hit_sound) {
		float random_pitch = 0.9f + (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.2f)));
		hit_sound->set_pitch_scale(random_pitch);
		hit_sound->play(0.0); 
	}
	if (sprite) {
		sprite->set_modulate(Color(1.0f, 0.2f, 0.2f, 1.0f));
		self->set_meta("flash_timer", 0.15f);
	}
	update_hp_bar();
}

float approach(float current, float target, float step) {
	if (current < target) return std::min(current + step, target);
	return std::max(current - step, target);
}

void OnAwake(Caller* instance) {
	self = GetSelf<CharacterBody2D>(instance);
	if (self) {
		self->add_to_group("enemy");
		sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
		ledge_check = Object::cast_to<RayCast2D>(self->get_node_or_null("LedgeCheck"));
		hp_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("ProgressBar"));
		
		hit_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("HitSound"));
		windup_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("WindupSound")); 
		smash_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("SmashSound"));   
		death_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("DeathSound"));   
		
		smash_indicator = memnew(ColorRect);
		smash_indicator->set_size(Vector2(smash_splash_radius * 2.0f, 10.0f)); 
		smash_indicator->set_color(Color(1.0f, 0.0f, 0.0f, 0.4f)); 
		smash_indicator->set_as_top_level(true); 
		smash_indicator->hide();
		self->add_child(smash_indicator);
	}
}

void OnReady(Caller* instance) {
	safely_clear_exception();
	current_state = STATE_PATROL;
	smash_cooldown = 2.0f; 
	health = MAX_HEALTH;
	is_dead = false;
	
	if (self) {
		self->set_meta("pending_damage", 0);
		self->set_meta("health", health); 
		self->set_meta("flash_timer", 0.0f);
	}

	if (hp_bar) {
		hp_bar->set_max(MAX_HEALTH);
		hp_bar->set_value(MAX_HEALTH);
		hp_bar->hide(); 
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self) return;

	Vector2 velocity = self->get_velocity();
	
	if (self->has_meta("flash_timer")) {
		float f_timer = (float)self->get_meta("flash_timer");
		if (f_timer > 0.0f) {
			f_timer -= (float)delta;
			self->set_meta("flash_timer", f_timer);
			if (f_timer <= 0.0f && !is_dead && sprite) sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	if (is_dead) {
		safely_clear_exception();
		if (current_state == STATE_DEATH) {
			if (hp_bar) hp_bar->hide();
			velocity.x = approach(velocity.x, 0.0f, ground_friction * (float)delta);
			state_timer -= (float)delta;
			if (sprite) sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, state_timer / 0.8f));
			if (state_timer <= 0.0f) self->queue_free();
			self->set_velocity(velocity);
			self->move_and_slide();
		}
		return;
	}

	if (self->has_meta("pending_damage") && (int)self->get_meta("pending_damage") > 0) {
		health -= (int)self->get_meta("pending_damage");
		self->set_meta("health", health);
		self->set_meta("pending_damage", 0);
		play_hit_effects();
		if (health <= 0) {
			is_dead = true; current_state = STATE_DEATH; state_timer = 0.8f;
			if (sprite) sprite->play("death");
			if (death_sound) death_sound->play(0.0); 
			return;
		}
	}

	if (smash_cooldown > 0.0f) smash_cooldown -= (float)delta;

	Node2D* player = nullptr;
	TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
	if (players.size() > 0) player = Object::cast_to<Node2D>(players[0]);

	switch (current_state) {
		case STATE_PATROL: {
			if (sprite) sprite->play("walk");
			if (self->is_on_wall() || (ledge_check && !ledge_check->is_colliding())) direction *= -1;
			velocity.x = approach(velocity.x, (float)direction * walk_speed, ground_acceleration * (float)delta);
			if (player && self->get_global_position().distance_to(player->get_global_position()) <= 250.0f) 
				current_state = STATE_CHASE;
			break;
		}

		case STATE_CHASE: {
			if (!player) { current_state = STATE_PATROL; break; }
			direction = (player->get_global_position().x > self->get_global_position().x) ? 1 : -1;
			if (smash_cooldown <= 0.0f) {
				current_state = STATE_SMASH_LAUNCH; state_timer = 1.2f; 
				if (sprite) sprite->play("jump_launch"); velocity.x = 0;
				if (windup_sound) windup_sound->play(0.0); 
			} else {
				if (sprite) sprite->play("walk");
				velocity.x = approach(velocity.x, (float)direction * run_speed, ground_acceleration * (float)delta);
			}
			break;
		}

		case STATE_SMASH_LAUNCH: {
			velocity.x = approach(velocity.x, 0.0f, ground_friction * (float)delta);
			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				current_state = STATE_SMASH_AIR;
				velocity.y = -700.0f; 
				target_smash_x = player ? player->get_global_position().x : self->get_global_position().x;
				if (sprite) sprite->play("jump_air"); 
				
				if (smash_indicator) { 
					smash_indicator->set_global_position(Vector2(target_smash_x - smash_splash_radius, fixed_floor_y)); 
					smash_indicator->show(); 
				}
				is_dive_bombing = false;

				if (player) {
					self->add_collision_exception_with(player);
					last_ignored_player = player;
				}
			}
			break;
		}

		case STATE_SMASH_AIR: {
			float x_diff = target_smash_x - self->get_global_position().x;
			if (std::abs(x_diff) > 10.0f && !is_dive_bombing) {
				velocity.x = approach(velocity.x, (x_diff > 0.0f ? 1.0f : -1.0f) * smash_horizontal_speed, 500.0f * (float)delta);
			} else {
				velocity.x = approach(velocity.x, 0.0f, 800.0f * (float)delta);
				if (velocity.y > 0.0f) is_dive_bombing = true;
			}
			velocity.y += (is_dive_bombing ? default_gravity * 3.5f : default_gravity) * (float)delta;

			if (self->is_on_floor() && velocity.y >= 0.0f) {
				current_state = STATE_SMASH_LAND;
				state_timer = 1.5f;
				if (smash_indicator) smash_indicator->hide(); 
				if (sprite) sprite->play("smash_land"); 
				if (smash_sound) smash_sound->play(0.0); 
				
				self->get_tree()->call_group("camera", "shake", 0.4f); 
				smash_cooldown = 4.0f; 

				safely_clear_exception();

				if (player) {
					float dist_to_impact = self->get_global_position().distance_to(player->get_global_position());
					if (dist_to_impact <= smash_splash_radius) {
						player->call("take_damage", smash_damage); 
						
						float splash_side = (player->get_global_position().x >= self->get_global_position().x) ? 1.0f : -1.0f;
						
						// PHYSICS BUG FIX: Lowered knockback velocity to avoid tunneling through tiles
						Vector2 safe_stable_knockback = Vector2(splash_side * 380.0f, -240.0f);
						player->call("apply_knockback", safe_stable_knockback);
					}
				}
			}
			break;
		}

		case STATE_SMASH_LAND: {
			velocity.x = 0;
			state_timer -= (float)delta;
			if (state_timer <= 0.0f) current_state = STATE_CHASE;
			break;
		}
	}

	if (!self->is_on_floor() && current_state != STATE_SMASH_AIR) velocity.y += default_gravity * (float)delta;
	if (sprite && std::abs(velocity.x) > 5.0f && current_state != STATE_SMASH_AIR) sprite->set_flip_h(velocity.x < 0.0f);
	self->set_velocity(velocity);
	self->move_and_slide();
}

void take_damage(Caller* instance, int amount) {
	if (is_dead) return;
	health -= amount; self->set_meta("health", health); play_hit_effects();
	if (health <= 0) { 
		is_dead = true; current_state = STATE_DEATH; state_timer = 0.8f; 
		if (sprite) sprite->play("death"); 
		if (death_sound) death_sound->play(0.0); 
	}
}

JENOVA_SCRIPT_END
