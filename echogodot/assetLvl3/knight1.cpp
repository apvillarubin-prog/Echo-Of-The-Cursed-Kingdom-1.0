#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/scene_tree_timer.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/classes/resource_loader.hpp>
#include <Godot/classes/packed_scene.hpp>
#include <Godot/classes/ray_cast2d.hpp>
#include <Godot/classes/line2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/progress_bar.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/string_name.hpp>
#include <Godot/variant/color.hpp>
#include <cmath>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
ProgressBar* health_bar = nullptr;

// --- Grappling Hook Node References ---
Line2D* grapple_line = nullptr;
Node2D* hand_anchor = nullptr; 

// --- Sound Effect Nodes ---
Node* walk_sfx = nullptr;
Node* block_sfx = nullptr;
Node* knight_attack_sfx = nullptr; 
Node* archer_attack_sfx = nullptr; 

Vector2 start_pos;
int inventory_count = 0;
bool is_dead = false;

// --- Combat Configurations ---
bool has_sword = true;
bool has_shield = false;
bool has_bow = true;  

// ACTION DURATIONS
float knight_attack_duration = 0.66f; 
float archer_attack_duration = 1.25f; 

bool is_blocking = false;
bool is_invisible = false; 
float block_timer = 0.0f;
float block_cooldown = 0.0f;
float BLOCK_DURATION = 2.0f;
int player_health = 50;
int last_attack_frame = -1;
int knight_damage = 10;

float damage_flash_timer = 0.0f;

// --- RESET: Wizard Custom Skill States ---
bool is_teleport_stance = false;
bool is_f_primed = false;
bool is_g_primed = false;

float wizard_q_cooldown = 0.0f;
float wizard_f_cooldown = 0.0f;
float wizard_g_cooldown = 0.0f;

// --- Grappling Hook Physics Variables ---
bool has_grapple = true; 
bool is_grappling = false;
Vector2 grapple_target_pos;
float grapple_radius = 0.0f;          
float grapple_climb_speed = 140.0f;    
float max_grapple_range = 160.0f; 

Vector2 hook_visual_offset = Vector2(0, 8); 

float grapple_launch_duration = 0.65f;  
float grapple_launch_timer = 0.0f; 

float grapple_jump_duration = 0.35f;  
float grapple_jump_timer = 0.0f;

bool unlocked_knight = true;
bool unlocked_archer = true;
bool unlocked_wizard = true; 

enum HeroType { KNIGHT = 0, ARCHER = 1, WIZARD = 2 }; 
HeroType current_hero = KNIGHT;

float speed = 90.0f;
float jump_velocity = -263.0f;
float gravity = 980.0f;

void unlock_sword(Caller* instance) { has_sword = true; Engine::get_singleton()->set_meta("save_has_sword", true); }
void unlock_shield(Caller* instance) { has_shield = true; Engine::get_singleton()->set_meta("save_has_shield", true); }
void unlock_bow(Caller* instance) { has_bow = true; Engine::get_singleton()->set_meta("save_has_bow", true); }
void unlock_grapple(Caller* instance) { has_grapple = true; Engine::get_singleton()->set_meta("save_has_grapple", true); }

void actually_teleport(Caller* instance);

void respawn() {
	if (is_dead) return;
	is_dead = true;
	is_grappling = false;
	is_invisible = false; 
	is_teleport_stance = false;
	is_f_primed = false;
	is_g_primed = false;
	
	if (grapple_line) grapple_line->set_visible(false);

	if (sprite) {
		String anim_name = (current_hero == WIZARD) ? "wizard_die" : (current_hero == KNIGHT ? "knight_death" : "archer_death");
		sprite->play(anim_name);
		sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f)); 
	}
	
	if (walk_sfx && (bool)walk_sfx->call("is_playing")) walk_sfx->call("stop");
	if (self) self->set_velocity(Vector2(0, 0));
	self->get_tree()->create_timer(1.0)->connect("timeout", Callable((Object*)self, "actually_teleport"));
}

bool take_damage(int amount) {
	if (is_dead || is_invisible) return false; 
	
	if (is_blocking && current_hero == KNIGHT) {
		if (block_timer > BLOCK_DURATION - 0.3f) {
			UtilityFunctions::print("[DEBUG] PERFECT PARRY!");
			return true; 
		}
		return false; 
	}
	
	player_health -= amount;
	damage_flash_timer = 0.15f; 
	if (sprite) sprite->set_modulate(Color(5.0f, 0.3f, 0.3f, 1.0f)); 

	if (health_bar) health_bar->set_value((double)player_health);
	if (player_health <= 0) respawn();
	
	return false; 
}

int increase_inventory(Caller* instance) { inventory_count += 1; return inventory_count; }
int get_inventory_count(Caller* instance) { return inventory_count; }

void actually_teleport(Caller* instance) {
	CharacterBody2D* player = GetSelf<CharacterBody2D>(instance);
	if (player) {
		Node* transition_manager = player->get_node_or_null("/root/TransitionManager");
		if (transition_manager) {
			transition_manager->call("fade_to_scene", "res://assetLvl3//DeathScreen.tscn");
		} else {
			SceneTree* tree = player->get_tree();
			if (tree) tree->reload_current_scene();
		}
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || is_dead) return;

	if (damage_flash_timer > 0.0f) {
		damage_flash_timer -= (float)delta;
		if (damage_flash_timer <= 0.0f && sprite) {
			sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	Input* input = Input::get_singleton();
	Vector2 velocity = self->get_velocity();

	// --- Custom Hardware Button Monitors ---
	bool curr_mouse_left = input->is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT);
	bool prev_mouse_left = self->has_meta("prev_mouse") ? (bool)self->get_meta("prev_mouse") : false;
	bool mouse_left_just_pressed = curr_mouse_left && !prev_mouse_left;
	self->set_meta("prev_mouse", curr_mouse_left);

	bool curr_key_q = input->is_key_pressed(Key::KEY_Q);
	bool prev_key_q = self->has_meta("prev_key_q") ? (bool)self->get_meta("prev_key_q") : false;
	bool key_q_just_pressed = curr_key_q && !prev_key_q;
	self->set_meta("prev_key_q", curr_key_q);

	bool curr_key_f = input->is_key_pressed(Key::KEY_F);
	bool prev_key_f = self->has_meta("prev_key_f") ? (bool)self->get_meta("prev_key_f") : false;
	bool key_f_just_pressed = curr_key_f && !prev_key_f;
	self->set_meta("prev_key_f", curr_key_f);

	bool curr_key_g = input->is_key_pressed(Key::KEY_G);
	bool prev_key_g = self->has_meta("prev_key_g") ? (bool)self->get_meta("prev_key_g") : false;
	bool key_g_just_pressed = curr_key_g && !prev_key_g;
	self->set_meta("prev_key_g", curr_key_g);

	float attack_timer = self->has_meta("attack_timer") ? (float)self->get_meta("attack_timer") : 0.0f;

	// Process Cooldowns
	if (wizard_q_cooldown > 0.0f) wizard_q_cooldown -= (float)delta;
	if (wizard_f_cooldown > 0.0f) wizard_f_cooldown -= (float)delta;
	if (wizard_g_cooldown > 0.0f) wizard_g_cooldown -= (float)delta;

	// --- Switching Heroes ---
	if (!is_grappling && !is_teleport_stance) {
		if (input->is_action_just_pressed("hero_1") && unlocked_knight) {
			current_hero = KNIGHT;
			is_f_primed = false; is_g_primed = false;
		}
		if (input->is_action_just_pressed("hero_2") && unlocked_archer) {
			current_hero = ARCHER;
			is_blocking = false;
			is_f_primed = false; is_g_primed = false;
		}
		if (input->is_action_just_pressed("hero_3") && unlocked_wizard) {
			current_hero = WIZARD;
			is_blocking = false;
		}
	}

	// ==========================================
	// WIZARD SKILL 1: Q TELEPORT STANCE
	// ==========================================
	if (current_hero == WIZARD) {
		if (key_q_just_pressed) {
			if (is_teleport_stance) {
				is_teleport_stance = false; 
			} else if (wizard_q_cooldown <= 0.0f) {
				is_teleport_stance = true;  
				is_f_primed = false;
				is_g_primed = false;
			}
		}

		if (is_teleport_stance) {
			velocity.x = 0.0f; // FIX: Only freeze horizontal movement
			if (mouse_left_just_pressed) {
				Vector2 click_target = self->get_global_mouse_position();
				self->set_global_position(click_target); 
				is_teleport_stance = false;             
				wizard_q_cooldown = 5.0f;               
			}
		}

		// ==========================================
		// WIZARD SKILL 2: F ICE FREEZE PRIMING
		// ==========================================
		if (key_f_just_pressed && wizard_f_cooldown <= 0.0f && !is_teleport_stance) {
			is_f_primed = true;
			is_g_primed = false;
		}

		if (is_f_primed && mouse_left_just_pressed) {
			Ref<PackedScene> ice_projectile = ResourceLoader::get_singleton()->load("res://assetLvl3/ice_freeze.tscn");
			if (ice_projectile.is_valid()) {
				Node* proj_inst = ice_projectile->instantiate();
				Node2D* proj_node = Object::cast_to<Node2D>(proj_inst);
				if (proj_node) {
					proj_node->set_meta("target_pos", self->get_global_mouse_position());
					self->get_tree()->get_current_scene()->add_child(proj_inst);
					proj_node->set_global_position(self->get_global_position());
				}
			}
			is_f_primed = false;
			wizard_f_cooldown = 5.0f;
			if (sprite) sprite->play("wizard_cast");
		}

		// ==========================================
		// WIZARD SKILL 3: G FIREBALL
		// ==========================================
		if (key_g_just_pressed && wizard_g_cooldown <= 0.0f && !is_teleport_stance) {
			is_g_primed = true;
			is_f_primed = false;
		}

		if (is_g_primed && mouse_left_just_pressed) {
			Ref<PackedScene> fireball_scene = ResourceLoader::get_singleton()->load("res://assetLvl3/fireball.tscn");
			if (fireball_scene.is_valid()) {
				Node* fb_inst = fireball_scene->instantiate();
				Node2D* fb_node = Object::cast_to<Node2D>(fb_inst);
				if (fb_node) {
					fb_node->set_meta("target_pos", self->get_global_mouse_position());
					self->get_tree()->get_current_scene()->add_child(fb_inst);
					fb_node->set_global_position(self->get_global_position());
				}
			}
			is_g_primed = false;
			wizard_g_cooldown = 60.0f;
			if (sprite) sprite->play("wizard_cast");
		}
	}

	// --- Grappling Hook Calculation Loop ---
	if (is_grappling && current_hero == ARCHER) {
		if (input->is_action_just_pressed("grapple_hook") || input->is_action_just_pressed("ui_accept")) {
			is_grappling = false;
			if (grapple_line) grapple_line->set_visible(false);
			if (input->is_action_just_pressed("ui_accept")) velocity.y = jump_velocity;
			self->set_velocity(velocity);
			self->move_and_slide();
			return;
		}

		if (grapple_launch_timer > 0.0f) {
			grapple_launch_timer -= (float)delta;
			velocity = Vector2(0, 0); 
			if (grapple_launch_timer <= 0.0f) {
				grapple_jump_timer = grapple_jump_duration;
				Vector2 current_pos = self->get_global_position();
				current_pos.y -= 18.0f; 
				self->set_global_position(current_pos);
				Vector2 to_target = grapple_target_pos - self->get_global_position();
				velocity = to_target.normalized() * 260.0f; 
				if (velocity.y > -140.0f) velocity.y = -180.0f; 
			}
		} 
		else if (grapple_jump_timer > 0.0f) {
			grapple_jump_timer -= (float)delta;
			velocity.y += gravity * (float)delta;
			if (grapple_jump_timer <= 0.0f) {
				grapple_radius = (self->get_global_position() - grapple_target_pos).length();
				if (grapple_radius < 30.0f) grapple_radius = 30.0f;
			}
		}
		else {
			float climb = (input->is_action_pressed("ui_up") || input->is_key_pressed(Key::KEY_W)) ? -1.0f : ((input->is_action_pressed("ui_down") || input->is_key_pressed(Key::KEY_S)) ? 1.0f : 0.0f);
			if (climb != 0.0f) {
				grapple_radius = UtilityFunctions::clamp(grapple_radius + climb * grapple_climb_speed * (float)delta, 30.0f, max_grapple_range);
			}

			velocity.y += gravity * (float)delta;
			Vector2 rope_dir = (self->get_global_position() - grapple_target_pos).normalized();
			Vector2 tangent = Vector2(-rope_dir.y, rope_dir.x).normalized();

			float swing = input->get_axis("ui_left", "ui_right");
			if (swing != 0.0f) {
				velocity += tangent * (-swing) * 280.0f * (float)delta; 
				if (sprite) sprite->set_flip_h(swing < 0);
			}
			velocity = tangent * (velocity.dot(tangent) * 0.990f) - rope_dir * ((self->get_global_position() - grapple_target_pos).length() - grapple_radius) * 25.0f;
		}

		self->set_velocity(velocity);
		self->move_and_slide();

		if (grapple_launch_timer <= 0.0f && (self->is_on_floor() || self->is_on_wall() || self->is_on_ceiling())) {
			is_grappling = false;
			if (grapple_line) grapple_line->set_visible(false);
			return;
		}

		if (grapple_launch_timer <= 0.0f && grapple_jump_timer <= 0.0f) {
			self->set_global_position(grapple_target_pos + (self->get_global_position() - grapple_target_pos).normalized() * grapple_radius);
		}

		if (grapple_line && hand_anchor) {
			grapple_line->set_visible(true);
			if (grapple_line->get_point_count() < 2) { grapple_line->clear_points(); grapple_line->add_point(Vector2(0,0)); grapple_line->add_point(Vector2(0,0)); }
			grapple_line->set_point_position(0, grapple_line->to_local(hand_anchor->get_global_position()));
			grapple_line->set_point_position(1, grapple_line->to_local(grapple_target_pos));
		}
		return; 
	}
	
	if (attack_timer > 0.0f) attack_timer -= (float)delta;

	// --- Knight Blocking Mechanics ---
	if (current_hero == KNIGHT && has_shield) {
		if (block_cooldown > 0.0f) block_cooldown -= (float)delta;
		if (key_q_just_pressed && !is_blocking && block_cooldown <= 0.0f && attack_timer <= 0.0f && self->is_on_floor()) {
			is_blocking = true; block_timer = BLOCK_DURATION; if (block_sfx) block_sfx->call("play");
		}
		if (is_blocking) {
			block_timer -= (float)delta;
			if (block_timer <= 0.0f) { is_blocking = false; block_cooldown = 1.0f; }
		}
	} else { is_blocking = false; }

	// --- Attack Handling Triggers ---
	bool is_attacking = false;
	if (attack_timer > 0.0f) {
		is_attacking = true; 
	} 
	else if (mouse_left_just_pressed && !is_blocking && current_hero != WIZARD) { 
		if (current_hero == KNIGHT && has_sword) {
			is_attacking = true; attack_timer = knight_attack_duration; 
			if (knight_attack_sfx) knight_attack_sfx->call("play");
		} else if (current_hero == ARCHER && has_bow) {
			is_attacking = true; attack_timer = archer_attack_duration; 
			if (archer_attack_sfx) archer_attack_sfx->call("play");
		}
	}
	self->set_meta("attack_timer", attack_timer);
	bool is_action_locked = is_attacking || is_blocking || is_teleport_stance;

	// Apply Core Engine Vector Shifts
	if (!self->is_on_floor()) {
		velocity.y += gravity * (float)delta; // FIX: Gravity is always applied
	}

	if (!is_teleport_stance) { // FIX: Teleport stance now only locks movement and jumping inputs
		if (input->is_action_just_pressed("ui_accept") && self->is_on_floor()) {
			velocity.y = jump_velocity; 
			if (is_attacking || is_blocking) { attack_timer = 0.0f; self->set_meta("attack_timer", 0.0f); is_attacking = false; is_blocking = false; is_action_locked = false; }
		}

		float direction = input->get_axis("ui_left", "ui_right");
		if (is_action_locked) {
			if (self->is_on_floor()) velocity.x = UtilityFunctions::move_toward(velocity.x, 0, speed * 6.0f * (float)delta); 
		} else {
			if (direction != 0) {
				velocity.x = direction * speed;
				if (sprite) sprite->set_flip_h(direction < 0);
			} else {
				velocity.x = UtilityFunctions::move_toward(velocity.x, 0, speed);
			}
		}
	}

	// --- Archer Grapple Initialization Trigger ---
	if (current_hero == ARCHER && has_grapple && input->is_action_just_pressed("grapple_hook") && !is_action_locked) {
		TypedArray<Node> targets = self->get_tree()->get_nodes_in_group("grapple_target");
		Node2D* nearest_valid_hook = nullptr;
		float shortest_distance = max_grapple_range; 

		for (int i = 0; i < targets.size(); i++) {
			Node2D* hook = Object::cast_to<Node2D>(targets[i]);
			if (hook) {
				float current_distance = self->get_global_position().distance_to(hook->get_global_position() + hook_visual_offset);
				if (current_distance < shortest_distance) { shortest_distance = current_distance; nearest_valid_hook = hook; }
			}
		}
		if (nearest_valid_hook) {
			is_grappling = true; grapple_launch_timer = grapple_launch_duration; grapple_jump_timer = 0.0f; 
			grapple_target_pos = nearest_valid_hook->get_global_position() + hook_visual_offset; 
			if (sprite) sprite->set_flip_h(grapple_target_pos.x < self->get_global_position().x);
		}
	}

	// --- Animation Engine Layout Updates ---
	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : (current_hero == ARCHER) ? "archer_" : "wizard_";

		if (is_blocking) {
			sprite->play("knight_block");
		} else if (is_attacking) {
			sprite->play(prefix + "attack");
			int current_frame = sprite->get_frame();
			int damage_frame = (current_hero == KNIGHT) ? 4 : 10;

			if (current_frame == damage_frame && last_attack_frame != damage_frame) {
				bool facing_right = !sprite->is_flipped_h();
				if (current_hero == KNIGHT) {
					TypedArray<Node> enemies = self->get_tree()->get_nodes_in_group("enemy");
					for (int i = 0; i < enemies.size(); i++) {
						Node2D* enemy = Object::cast_to<Node2D>(enemies[i]);
						if (enemy) {
							Vector2 diff = enemy->get_global_position() - self->get_global_position();
							if ((facing_right ? (diff.x > 0) : (diff.x < 0)) && std::abs(diff.x) < 60.0f && std::abs(diff.y) < 32.5f) {
								enemy->set_meta("pending_damage", knight_damage);
							}
						}
					}
				} else if (current_hero == ARCHER) {
					Ref<PackedScene> arrow_scene = ResourceLoader::get_singleton()->load("res://scene/arrow.tscn");
					if (arrow_scene.is_valid()) {
						Node* arrow_instance = arrow_scene->instantiate();
						Node2D* arrow = Object::cast_to<Node2D>(arrow_instance);
						if (arrow) {
							self->get_tree()->get_current_scene()->add_child(arrow_instance);
							arrow->set_global_position(self->get_global_position() + (facing_right ? Vector2(20, -12) : Vector2(-20, -12)));
							arrow->set_meta("facing_left", !facing_right);
							arrow->set_scale(Vector2(0.5f, 0.5f)); 
						}
					}
				}
			}
			last_attack_frame = current_frame;
		} else {
			last_attack_frame = -1;
			if (is_teleport_stance) {
				sprite->play("wizard_idle"); 
			} else if (!self->is_on_floor()) {
				sprite->play(prefix + "jump");
			} else if (velocity.x != 0) {
				sprite->play(prefix + "walk");
				if (walk_sfx && !(bool)walk_sfx->call("is_playing") && current_hero != WIZARD) walk_sfx->call("play");
			} else {
				sprite->play(prefix + "idle");
				if (walk_sfx && (bool)walk_sfx->call("is_playing")) walk_sfx->call("stop");
			}
		}
	}

	self->set_velocity(velocity);
	self->move_and_slide();
}

void OnAwake(Caller* instance) { self = GetSelf<CharacterBody2D>(instance); if (self) { self->add_to_group("player"); for (int i = 0; i < self->get_child_count(); i++) { Node* child = self->get_child(i); if (Object::cast_to<AnimatedSprite2D>(child)) { sprite = (AnimatedSprite2D*)child; break; } } } }

void OnReady(Caller* instance) { 
	if (self) { 
		start_pos = self->get_global_position(); 
		is_dead = false; 
		is_grappling = false; 
		is_teleport_stance = false; 
		is_f_primed = false; 
		is_g_primed = false; 
		player_health = 50; 
		
		grapple_line = Object::cast_to<Line2D>(self->get_node_or_null("GrappleLine"));
		hand_anchor = Object::cast_to<Node2D>(self->get_node_or_null("HandAnchor"));

		health_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("CanvasLayer/HealthBar")); 
		if (health_bar) { health_bar->set_max(50.0); health_bar->set_value(50.0); } 
		
		walk_sfx = self->get_node_or_null("WalkSFX"); 
		block_sfx = self->get_node_or_null("BlockSFX"); 
		knight_attack_sfx = self->get_node_or_null("AttackSFX"); 
		archer_attack_sfx = self->get_node_or_null("ArcherAttackSFX"); 
	} 
}

void apply_knockback(Caller* instance, Vector2 force) {
	if (!self) return;

	// Apply the knockback force directly to velocity
	self->set_velocity(force);

	// Force an immediate physics update
	self->move_and_slide();
}

JENOVA_SCRIPT_END
