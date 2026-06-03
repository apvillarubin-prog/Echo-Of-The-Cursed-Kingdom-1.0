#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/ray_cast2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/string_name.hpp>
#include <Godot/variant/color.hpp> // Added for visual flash color processing

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
RayCast2D* ledge_check = nullptr;

enum BruteState { STATE_PATROL, STATE_TELEGRAPH, STATE_CHARGE, STATE_COOLDOWN, STATE_DEATH };
BruteState current_state = STATE_PATROL;

// --- Tuning Variables ---
int enemy_health = 40;
int charge_damage = 15;
float gravity = 980.0f;

float patrol_speed = 35.0f;
float charge_speed = 160.0f;
float detection_range = 210.0f; 

float state_timer = 0.0f;
float telegraph_duration = 1.8f;
float max_charge_duration = 1.0f;
float cooldown_duration = 3.0f; 

int direction = -1; 
bool has_dealt_damage_this_charge = false;

void OnAwake(Caller* instance) {
	self = GetSelf<CharacterBody2D>(instance);
	if (self) {
		self->add_to_group("enemy");
		sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
		ledge_check = Object::cast_to<RayCast2D>(self->get_node_or_null("LedgeCheck"));
	}
}

void OnReady(Caller* instance) {
	current_state = STATE_PATROL;
	if (self) {
		self->set_meta("pending_damage", 0);
		self->set_meta("flash_timer", 0.0f); // Initialize flash timer
	}
	has_dealt_damage_this_charge = false;
	if (sprite) sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Ensure color resets on spawn
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self) return;

	Vector2 velocity = self->get_velocity();

	// --- FLASH TIMER LOGIC ---
	if (self->has_meta("flash_timer")) {
		float f_timer = (float)self->get_meta("flash_timer");
		if (f_timer > 0.0f) {
			f_timer -= (float)delta;
			self->set_meta("flash_timer", f_timer);
			// Reset color back to normal only if not dead
			if (f_timer <= 0.0f && current_state != STATE_DEATH && sprite) {
				sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	}

	// --- 1. DEATH STATE OVERRIDE ---
	if (current_state == STATE_DEATH) {
		if (!self->is_on_floor()) {
			velocity.y += gravity * (float)delta;
		} else {
			velocity.y = 0;
		}
		velocity.x = 0;

		state_timer -= (float)delta;
		if (state_timer <= 0.0f) {
			self->queue_free(); 
			return;
		}

		self->set_velocity(velocity);
		self->move_and_slide();
		return; 
	}
	
	// --- 2. HEALTH AND INCOMING DAMAGE SYSTEM ---
	if (self->has_meta("pending_damage") && (int)self->get_meta("pending_damage") > 0) {
		int dmg = self->get_meta("pending_damage");
		enemy_health -= dmg;
		self->set_meta("pending_damage", 0);
		
		// Trigger Flash Highlight (Orc Style)
		if (sprite && enemy_health > 0) {
			sprite->set_modulate(Color(10.0f, 2.0f, 2.0f, 1.0f)); 
			self->set_meta("flash_timer", 0.15f);
		}
		
		if (enemy_health <= 0) {
			current_state = STATE_DEATH;
			state_timer = 0.75f; 
			
			if (sprite) {
				sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Clean reset color filter for death anim
				sprite->play("death");
			}
			
			self->remove_from_group("enemy"); 
			
			velocity = Vector2(0, 0);
			self->set_velocity(velocity);
			self->move_and_slide();
			return;
		}
	}

	if (!self->is_on_floor()) {
		velocity.y += gravity * (float)delta;
	} else {
		velocity.y = 0;
	}

	Node2D* player = nullptr;
	TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
	if (players.size() > 0) {
		player = Object::cast_to<Node2D>(players[0]);
	}

	// --- 3. STATE MACHINE ROUTINES ---
	switch (current_state) {
		
		case STATE_PATROL: {
			if (sprite && sprite->get_animation() != StringName("walk")) sprite->play("walk");
			
			bool walking_off_ledge = ledge_check && !ledge_check->is_colliding();
			if (self->is_on_wall() || (self->is_on_floor() && walking_off_ledge)) {
				direction *= -1;
				if (ledge_check) ledge_check->set_position(Vector2(direction * 15.0f, 0));
			}
			velocity.x = direction * patrol_speed;

			if (player) {
				float distance_to_player = self->get_global_position().distance_to(player->get_global_position());
				if (distance_to_player <= detection_range) {
					direction = (player->get_global_position().x > self->get_global_position().x) ? 1 : -1;
					if (ledge_check) ledge_check->set_position(Vector2(direction * 15.0f, 0));

					current_state = STATE_TELEGRAPH;
					state_timer = telegraph_duration;
					velocity.x = 0;
				}
			}
			break;
		}

		case STATE_TELEGRAPH: {
			if (sprite && sprite->get_animation() != StringName("telegraph")) sprite->play("telegraph");
			velocity.x = 0;

			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				if (player) {
					direction = (player->get_global_position().x > self->get_global_position().x) ? 1 : -1;
				}
				current_state = STATE_CHARGE;
				state_timer = max_charge_duration;
				has_dealt_damage_this_charge = false; 
			}
			break;
		}

		case STATE_CHARGE: {
			if (sprite && sprite->get_animation() != StringName("charge")) sprite->play("charge");
			
			velocity.x = direction * charge_speed;

			if (self->is_on_wall() && state_timer < (max_charge_duration - 0.15f)) {
				current_state = STATE_COOLDOWN;
				state_timer = cooldown_duration;
				velocity.x = 0;
				break;
			}

			if (player && !has_dealt_damage_this_charge) {
				Vector2 player_pos = player->get_global_position();
				Vector2 brute_pos = self->get_global_position();

				float diff_x = UtilityFunctions::abs(player_pos.x - brute_pos.x);
				float player_vertical_clearance = brute_pos.y - player_pos.y;

				if (diff_x < 34.0f && player_vertical_clearance < 16.0f) {
					player->call("take_damage", charge_damage);
					has_dealt_damage_this_charge = true; 
				}
			}

			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				current_state = STATE_COOLDOWN;
				state_timer = cooldown_duration;
				velocity.x = 0;
			}
			break;
		}

		case STATE_COOLDOWN: {
			if (sprite && sprite->get_animation() != StringName("cooldown")) sprite->play("cooldown");
			velocity.x = 0;

			state_timer -= (float)delta;
			if (state_timer <= 0.0f) {
				current_state = STATE_PATROL; 
			}
			break;
		}
		
		case STATE_DEATH:
			break;
	}

	if (sprite) {
		sprite->set_flip_h(direction > 0);
	}

	self->set_velocity(velocity);
	self->move_and_slide();
}

JENOVA_SCRIPT_END
