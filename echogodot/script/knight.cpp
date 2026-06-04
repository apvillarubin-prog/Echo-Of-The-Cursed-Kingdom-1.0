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
#include <Godot/classes/audio_stream_player.hpp> 
#include <Godot/classes/control.hpp> 
#include <Godot/classes/button.hpp>  
#include <Godot/classes/kinematic_collision2d.hpp>
#include <Godot/classes/rigid_body2d.hpp>         
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/string_name.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
ProgressBar* health_bar = nullptr;

// --- Admin Toggle ---
bool admin_mode = false; // <-- SET TO FALSE FOR PRODUCTION

// --- Death Screen Pointers ---
Control* death_screen = nullptr;

// --- Audio Pointers ---
AudioStreamPlayer* bg_music = nullptr;
AudioStreamPlayer* combat_music = nullptr;
AudioStreamPlayer* sfx_walk = nullptr;
AudioStreamPlayer* sfx_attack = nullptr;
AudioStreamPlayer* sfx_block = nullptr;
AudioStreamPlayer* sfx_landing = nullptr; 
AudioStreamPlayer* sfx_defeat = nullptr;
AudioStreamPlayer* sfx_archer_attack = nullptr;
AudioStreamPlayer* sfx_priest_heal = nullptr; 
AudioStreamPlayer* sfx_priest_buff = nullptr;
AudioStreamPlayer* sfx_wizard_summon = nullptr; 
AudioStreamPlayer* sfx_wizard_teleport = nullptr;

bool was_on_floor = true; 
float combat_cooldown = 0.0f; 

Vector2 start_pos;
int inventory_count = 0;
bool is_dead = false;

// --- Combat Configurations ---
bool has_sword = false;
bool has_shield = false;
bool has_bow = false;
bool has_heal = false;
bool has_buff = false;  

// --- Wizard Configurations ---
bool has_summon = false;
bool has_teleport = false;

float wizard_summon_cooldown_val = 0.0f;
float wizard_teleport_cooldown_val = 0.0f;
float wizard_action_timer = 0.0f;
bool is_summoning = false;
bool is_teleporting = false;

// Timings matched exactly to Godot Animation player
float knight_attack_duration = 0.66f; 
float archer_attack_duration = 1.08f; 

bool is_blocking = false;
float block_timer = 0.0f;
float block_cooldown_val = 0.0f;
float BLOCK_DURATION = 1.5f;
int player_health = 50;
int last_attack_frame = -1;
int knight_damage = 10;

// --- Priest Configurations ---
float priest_heal_cooldown_val = 0.0f;
float priest_buff_cooldown_val = 0.0f;
float buff_active_timer = 0.0f; 
float priest_action_timer = 0.0f; 
bool is_healing = false;
bool is_buffing = false;

bool has_grapple = false; 
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
bool unlocked_archer = false;
bool unlocked_priest = false;
bool unlocked_wizard = false;

enum HeroType { KNIGHT = 0, ARCHER = 1, PRIEST = 2, WIZARD = 3 };
HeroType current_hero = KNIGHT;

float speed = 70.0f;
float jump_velocity = -253.0f;
float gravity = 980.0f;

void unlock_sword(Caller* instance) { has_sword = true; Engine::get_singleton()->set_meta("save_has_sword", true); }
void unlock_shield(Caller* instance) { has_shield = true; Engine::get_singleton()->set_meta("save_has_shield", true); }
void unlock_bow(Caller* instance) { has_bow = true; Engine::get_singleton()->set_meta("save_has_bow", true); }
void unlock_grapple(Caller* instance) { has_grapple = true; Engine::get_singleton()->set_meta("save_has_grapple", true); }
void unlock_heal(Caller* instance) { has_heal = true; Engine::get_singleton()->set_meta("save_has_heal", true); }
void unlock_buff(Caller* instance) { has_buff = true; Engine::get_singleton()->set_meta("save_has_buff", true); }
void unlock_summon(Caller* instance) { has_summon = true; Engine::get_singleton()->set_meta("save_has_summon", true); }
void unlock_teleport(Caller* instance) { has_teleport = true; Engine::get_singleton()->set_meta("save_has_teleport", true); }

void actually_teleport(Caller* instance);

void respawn() {
	if (is_dead) return;
	is_dead = true;
	is_grappling = false;
	if (sfx_defeat) sfx_defeat->play();
	UtilityFunctions::print("[DEBUG-DEATH] Player died. Triggering respawn logic.");

	Line2D* rope = Object::cast_to<Line2D>(self->get_node_or_null("GrappleLine"));
	if (rope) rope->set_visible(false);

	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : 
						(current_hero == ARCHER) ? "archer_" : 
						(current_hero == PRIEST) ? "priest_" : "wizard_";
		sprite->play(prefix + "death");
	}
	if (self) self->set_velocity(Vector2(0, 0));
	
	if (death_screen) {
		death_screen->set_visible(true);
		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
		UtilityFunctions::print("[DEBUG-DEATH] Death screen made visible. Mouse unlocked.");
	} else {
		UtilityFunctions::print("[DEBUG-DEATH] WARNING: No death screen found. Using 1-sec fallback timer.");
		self->get_tree()->create_timer(1.0)->connect("timeout", Callable((Object*)self, "actually_teleport"));
	}
}

bool take_damage(int amount) {
	if (is_dead) return false;
	
	if (is_blocking && current_hero == KNIGHT) {
		if (block_timer > BLOCK_DURATION - 0.3f) {
			return true; // Perfect Parry
		}
		amount = amount / 2;
	}
	
	int old_hp = player_health;
	player_health -= amount;

	if (self) self->set_meta("health", player_health);
	if (health_bar) health_bar->set_value((double)player_health);
	
	UtilityFunctions::print(String("[DEBUG-PLAYER] Player took ") + String::num_int64(amount) + String(" damage. HP: ") + String::num_int64(old_hp) + String(" -> ") + String::num_int64(player_health));
	
	if (player_health <= 0) respawn();
	
	return false; 
}

int increase_inventory(Caller* instance) { inventory_count += 1; return inventory_count; }
int get_inventory_count(Caller* instance) { return inventory_count; }

void actually_teleport(Caller* instance) {
	UtilityFunctions::print("[DEBUG-DEATH] actually_teleport() FIRED!");
	
	CharacterBody2D* player = GetSelf<CharacterBody2D>(instance);
	if (player) {
		player->set_global_position(start_pos);
		is_dead = false;
		player_health = 50;

		player->set_meta("health", 50);

		// --- [NEW] Reset all spikes in the level ---
		TypedArray<Node> spikes = player->get_tree()->get_nodes_in_group("spikes");
		for (int i = 0; i < spikes.size(); i++) {
			Node* spike = Object::cast_to<Node>(spikes[i]);
			if (spike) {
				spike->set_meta("already_triggered", false);
			}
		}
		// -------------------------------------------

		if (health_bar) health_bar->set_value(50.0);
		is_blocking = false;
		block_cooldown_val = 0.0f;
		
		is_healing = false;
		is_buffing = false;
		is_summoning = false;
		is_teleporting = false;
		
		priest_action_timer = 0.0f;
		wizard_action_timer = 0.0f;
		buff_active_timer = 0.0f;
		priest_heal_cooldown_val = 0.0f;
		priest_buff_cooldown_val = 0.0f;
		wizard_summon_cooldown_val = 0.0f;
		wizard_teleport_cooldown_val = 0.0f;

		if (death_screen) death_screen->set_visible(false);
		if (sprite) {
			String prefix = (current_hero == KNIGHT) ? "knight_" : 
							(current_hero == ARCHER) ? "archer_" : 
							(current_hero == PRIEST) ? "priest_" : "wizard_";
			sprite->play(prefix + "idle");
		}
		UtilityFunctions::print("[DEBUG-DEATH] Player successfully reset to start_pos.");
	}
}

void OnAwake(Caller* instance) {
	self = GetSelf<CharacterBody2D>(instance);
	if (self) {
		self->add_to_group("player");
		for (int i = 0; i < self->get_child_count(); i++) {
			Node* child = self->get_child(i);
			if (Object::cast_to<AnimatedSprite2D>(child)) {
				sprite = (AnimatedSprite2D*)child;
				break;
			}
		}
	}
}

void OnReady(Caller* instance) {
	if (self) {
		inventory_count = 0; 
		start_pos = self->get_global_position();
		is_dead = false;
		is_grappling = false;
		grapple_launch_timer = 0.0f;
		grapple_jump_timer = 0.0f;
		
		self->set_meta("prev_mouse", false);
		self->set_meta("prev_key_q", false);
		self->set_meta("prev_key_e", false);
		self->set_meta("attack_timer", 0.0f);
		
		player_health = 50;
		self->set_meta("health", 50);

		health_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("CanvasLayer/HealthBar"));
		if (health_bar) {
			health_bar->set_max(50.0);
			health_bar->set_value(50.0);
		}

		death_screen = Object::cast_to<Control>(self->get_node_or_null("CanvasLayer/DeathScreen"));
		if (death_screen) {
			death_screen->set_visible(false);
			Button* respawn_btn = Object::cast_to<Button>(death_screen->get_node_or_null("RespawnButton"));
			if (respawn_btn) {
				respawn_btn->set_focus_mode(Control::FOCUS_ALL);
				if (!respawn_btn->is_connected("pressed", Callable((Object*)self, "actually_teleport"))) {
					respawn_btn->connect("pressed", Callable((Object*)self, "actually_teleport"));
				}
			}
		}

		bg_music = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("LevelBGM"));
		combat_music = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("CombatBGM"));
		sfx_walk = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("WalkSFX"));
		sfx_attack = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("AttackSFX"));
		sfx_block = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("BlockSFX"));
		sfx_defeat = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("defeat"));
		sfx_archer_attack = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("archerattack"));
		sfx_priest_heal = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("heal"));
		sfx_priest_buff = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("buff"));
		sfx_landing = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("landing"));
		sfx_wizard_summon = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("summon"));
		sfx_wizard_teleport = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("teleport"));
		was_on_floor = self->is_on_floor(); 

		if (bg_music) {
			bg_music->set_volume_db(0.0f);
			bg_music->play();
		}
		if (combat_music) {
			combat_music->set_volume_db(-60.0f); 
			combat_music->play();
		}

		if (admin_mode) {
			unlocked_knight = true;
			unlocked_archer = true;
			unlocked_priest = true;
			unlocked_wizard = true;
			has_sword = true;
			has_shield = true;
			has_bow = true;
			has_grapple = true;
			has_heal = true;
			has_buff = true;
			has_summon = true;
			has_teleport = true;
			UtilityFunctions::print("[DEBUG] Admin mode is ON: All characters and weapons unlocked.");
		} else {
			Engine* engine = Engine::get_singleton();
			has_sword = engine->has_meta("save_has_sword") ? (bool)engine->get_meta("save_has_sword") : false;
			has_shield = engine->has_meta("save_has_shield") ? (bool)engine->get_meta("save_has_shield") : false;
			has_bow = engine->has_meta("save_has_bow") ? (bool)engine->get_meta("save_has_bow") : false;
			has_grapple = engine->has_meta("save_has_grapple") ? (bool)engine->get_meta("save_has_grapple") : false;
			has_heal = engine->has_meta("save_has_heal") ? (bool)engine->get_meta("save_has_heal") : false;
			has_buff = engine->has_meta("save_has_buff") ? (bool)engine->get_meta("save_has_buff") : false;
			has_summon = engine->has_meta("save_has_summon") ? (bool)engine->get_meta("save_has_summon") : false;
			has_teleport = engine->has_meta("save_has_teleport") ? (bool)engine->get_meta("save_has_teleport") : false;
			
			String scene_name = self->get_tree()->get_current_scene()->get_name();
			
			if (scene_name == "level2" || scene_name == "Level2") {
				unlocked_archer = true; 
				has_sword = true;
				has_shield = true;
				unlocked_priest = false;
				unlocked_wizard = false;
			} else if (scene_name == "level3" || scene_name == "Level3") {
				unlocked_archer = true; 
				unlocked_wizard = true;
				has_sword = true;
				has_shield = true;
				unlocked_priest = false;
			} else if (scene_name == "level4" || scene_name == "Level4") {
				unlocked_archer = true; 
				unlocked_priest = true;
				unlocked_wizard = true;
				has_sword = true;
				has_shield = true;
			} else {
				unlocked_archer = false; 
				unlocked_priest = false;
				unlocked_wizard = false;
			}
		}
		
		current_hero = KNIGHT;
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || is_dead) return;

	Input* input = Input::get_singleton();
	Vector2 velocity = self->get_velocity();

	bool curr_mouse_left = input->is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT);
	bool prev_mouse_left = self->has_meta("prev_mouse") ? (bool)self->get_meta("prev_mouse") : false;
	bool mouse_left_just_pressed = curr_mouse_left && !prev_mouse_left;
	self->set_meta("prev_mouse", curr_mouse_left);

	bool curr_key_q = input->is_key_pressed(Key::KEY_Q);
	bool prev_key_q = self->has_meta("prev_key_q") ? (bool)self->get_meta("prev_key_q") : false;
	bool key_q_just_pressed = curr_key_q && !prev_key_q;
	self->set_meta("prev_key_q", curr_key_q);
	
	bool curr_key_e = input->is_key_pressed(Key::KEY_E);
	bool prev_key_e = self->has_meta("prev_key_e") ? (bool)self->get_meta("prev_key_e") : false;
	bool key_e_just_pressed = curr_key_e && !prev_key_e;
	self->set_meta("prev_key_e", curr_key_e);

	float attack_timer = self->has_meta("attack_timer") ? (float)self->get_meta("attack_timer") : 0.0f;

	if (priest_heal_cooldown_val > 0.0f) priest_heal_cooldown_val -= (float)delta;
	if (priest_buff_cooldown_val > 0.0f) priest_buff_cooldown_val -= (float)delta;
	if (buff_active_timer > 0.0f) buff_active_timer -= (float)delta;
	
	if (wizard_summon_cooldown_val > 0.0f) wizard_summon_cooldown_val -= (float)delta;
	if (wizard_teleport_cooldown_val > 0.0f) wizard_teleport_cooldown_val -= (float)delta;
	
	knight_damage = (buff_active_timer > 0.0f) ? 20 : 10;
	
	if (priest_action_timer > 0.0f) {
		priest_action_timer -= (float)delta;
		if (priest_action_timer <= 0.0f) {
			is_healing = false;
			is_buffing = false;
		}
	}
	
	if (wizard_action_timer > 0.0f) {
		wizard_action_timer -= (float)delta;
		if (wizard_action_timer <= 0.0f) {
			is_summoning = false;
			is_teleporting = false;
		}
	}

	bool enemies_near = false;
	TypedArray<Node> enemies = self->get_tree()->get_nodes_in_group("enemy");
	for (int i = 0; i < enemies.size(); i++) {
		Node2D* enemy = Object::cast_to<Node2D>(enemies[i]);
		if (enemy && self->get_global_position().distance_to(enemy->get_global_position()) < 150.0f) {
			enemies_near = true;
			break;
		}
	}

	if (enemies_near) {
		combat_cooldown = 2.0f; 
	} else if (combat_cooldown > 0.0f) {
		combat_cooldown -= (float)delta;
	}

	if (bg_music && combat_music) {
		bool play_combat = (combat_cooldown > 0.0f);
		float target_bg_vol = play_combat ? -15.0f : 0.0f;     
		float target_combat_vol = play_combat ? 0.0f : -60.0f; 

		float current_bg = bg_music->get_volume_db();
		float current_combat = combat_music->get_volume_db();
		
		bg_music->set_volume_db(current_bg + (target_bg_vol - current_bg) * 2.0f * (float)delta);
		combat_music->set_volume_db(current_combat + (target_combat_vol - current_combat) * 2.0f * (float)delta);
	}

	if (!is_grappling) {
		if (input->is_action_just_pressed("hero_1") && unlocked_knight) current_hero = KNIGHT;
		if (input->is_action_just_pressed("hero_2") && unlocked_archer) { current_hero = ARCHER; is_blocking = false; }
		if (input->is_action_just_pressed("hero_3") && unlocked_wizard) current_hero = WIZARD;
		if (input->is_action_just_pressed("hero_4") && unlocked_priest) current_hero = PRIEST;
	}

	Line2D* grapple_line = Object::cast_to<Line2D>(self->get_node_or_null("GrappleLine"));
	Node2D* hand_anchor = Object::cast_to<Node2D>(self->get_node_or_null("BowHandAnchor"));

	if (is_grappling && current_hero == ARCHER) {
		if (input->is_action_just_pressed("grapple_hook")) {
			is_grappling = false;
			if (grapple_line) grapple_line->set_visible(false);
			self->set_velocity(velocity);
			self->move_and_slide();
			return;
		}

		if (input->is_action_just_pressed("ui_accept")) {
			is_grappling = false;
			if (grapple_line) grapple_line->set_visible(false);
			velocity.y = jump_velocity; 
			self->set_velocity(velocity);
			self->move_and_slide();
			return;
		}

		if (grapple_launch_timer > 0.0f) {
			grapple_launch_timer -= (float)delta;
			velocity = Vector2(0, 0); 
			
			if (sprite && sprite->get_animation() != StringName("archer_grapple_launch")) {
				sprite->play("archer_grapple_launch"); 
			}
			if (grapple_line) grapple_line->set_visible(false);

			if (grapple_launch_timer <= 0.0f) {
				grapple_jump_timer = grapple_jump_duration;
				Vector2 current_pos = self->get_global_position();
				current_pos.y -= 18.0f; 
				self->set_global_position(current_pos);
				
				if (sprite) sprite->play("archer_jump");
				
				Vector2 to_target = grapple_target_pos - self->get_global_position();
				velocity = to_target.normalized() * 260.0f; 
				if (velocity.y > -140.0f) velocity.y = -180.0f; 
			}
		} 
		else if (grapple_jump_timer > 0.0f) {
			grapple_jump_timer -= (float)delta;
			if (sprite && sprite->get_animation() != StringName("archer_jump")) sprite->play("archer_jump"); 
			velocity.y += gravity * (float)delta;

			if (grapple_jump_timer <= 0.0f) {
				Vector2 rope_vector = self->get_global_position() - grapple_target_pos;
				grapple_radius = rope_vector.length();
				if (grapple_radius < 30.0f) grapple_radius = 30.0f;
			}
		}
		else {
			bool input_up = input->is_action_pressed("ui_up") || input->is_key_pressed(Key::KEY_W);
			bool input_down = input->is_action_pressed("ui_down") || input->is_key_pressed(Key::KEY_S);
			float climb_direction = 0.0f;
			if (input_up) climb_direction = -1.0f;
			if (input_down) climb_direction = 1.0f;

			if (climb_direction != 0.0f) {
				grapple_radius += climb_direction * grapple_climb_speed * (float)delta;
				if (grapple_radius < 30.0f) grapple_radius = 30.0f;
				if (grapple_radius > max_grapple_range) grapple_radius = max_grapple_range;
				if (sprite && sprite->get_animation() != StringName("archer_grapple_pull")) sprite->play("archer_grapple_pull"); 
			} 
			else {
				if (sprite && sprite->get_animation() != StringName("archer_swing")) sprite->play("archer_swing"); 
			}

			velocity.y += gravity * (float)delta;
			Vector2 rope_vector = self->get_global_position() - grapple_target_pos;
			Vector2 rope_dir = rope_vector.normalized();
			Vector2 tangent_trajectory = Vector2(-rope_dir.y, rope_dir.x).normalized();

			float swing_input = input->get_axis("ui_left", "ui_right");
			if (swing_input != 0.0f) {
				velocity += tangent_trajectory * (-swing_input) * 280.0f * (float)delta; 
				if (sprite) sprite->set_flip_h(swing_input < 0);
			}

			float tangent_speed = velocity.dot(tangent_trajectory);
			tangent_speed *= 0.990f; 
			velocity = tangent_trajectory * tangent_speed;
			float current_dist = rope_vector.length();
			velocity -= rope_dir * (current_dist - grapple_radius) * 25.0f;
		}

		self->set_velocity(velocity);
		self->move_and_slide();

		if (is_grappling && grapple_launch_timer <= 0.0f) {
			if (self->is_on_floor() || self->is_on_wall() || self->is_on_ceiling()) {
				is_grappling = false;
				if (grapple_line) grapple_line->set_visible(false);
				return;
			}
		}

		if (is_grappling && grapple_launch_timer <= 0.0f && grapple_jump_timer <= 0.0f) {
			Vector2 corrected_offset = self->get_global_position() - grapple_target_pos;
			self->set_global_position(grapple_target_pos + corrected_offset.normalized() * grapple_radius);
		}

		if (is_grappling && grapple_launch_timer <= 0.0f && grapple_line && hand_anchor) {
			grapple_line->set_visible(true);
			if (grapple_line->get_point_count() < 2) {
				grapple_line->clear_points();
				grapple_line->add_point(Vector2(0,0));
				grapple_line->add_point(Vector2(0,0));
			}
			grapple_line->set_point_position(0, grapple_line->to_local(hand_anchor->get_global_position()));
			grapple_line->set_point_position(1, grapple_line->to_local(grapple_target_pos));
		}
		return; 
	}
	
	if (attack_timer > 0.0f) attack_timer -= (float)delta;
	
	bool is_action_locked = (attack_timer > 0.0f) || is_blocking || is_healing || is_buffing || is_summoning || is_teleporting;

	if (current_hero == KNIGHT && has_shield) {
		if (block_cooldown_val > 0.0f) block_cooldown_val -= (float)delta;
		if (key_q_just_pressed && !is_blocking && block_cooldown_val <= 0.0f && attack_timer <= 0.0f && self->is_on_floor()) {
			is_blocking = true;
			block_timer = BLOCK_DURATION;
			if (sfx_block) sfx_block->play();
			is_action_locked = true;
		}
		if (is_blocking) {
			block_timer -= (float)delta;
			if (block_timer <= 0.0f) {
				is_blocking = false;
				block_cooldown_val = 1.0f;
			}
		}
	} else {
		is_blocking = false;
	}
	
	if (current_hero == PRIEST && !is_action_locked && self->is_on_floor()) {
		if (key_e_just_pressed && has_heal && priest_heal_cooldown_val <= 0.0f) {
			is_healing = true;
			priest_action_timer = 1.2f; 
			priest_heal_cooldown_val = 3.0f;
			if (sfx_priest_heal) sfx_priest_heal->play();
			
			int old_hp = player_health;
			player_health += 10;
			if (player_health > 50) player_health = 50;

			self->set_meta("health", player_health);
			if (health_bar) health_bar->set_value((double)player_health);
			is_action_locked = true;
			
			UtilityFunctions::print(String("[DEBUG-PLAYER] Priest Heal! HP: ") + String::num_int64(old_hp) + String(" -> ") + String::num_int64(player_health));
		} 
		else if (key_q_just_pressed && has_buff && priest_buff_cooldown_val <= 0.0f) {
			is_buffing = true;
			priest_action_timer = 1.2f;
			priest_buff_cooldown_val = 3.0f;
			buff_active_timer = 10.0f;
			if (sfx_priest_buff) sfx_priest_buff->play(); 
			is_action_locked = true;
			UtilityFunctions::print("[DEBUG-PLAYER] Priest Buff Applied! Knight Damage Doubled.");
		}
	}
	
	if (current_hero == WIZARD && !is_action_locked && self->is_on_floor()) {
		if (mouse_left_just_pressed && has_summon && wizard_summon_cooldown_val <= 0.0f) {
			is_summoning = true;
			wizard_action_timer = 1.6f; 
			wizard_summon_cooldown_val = 3.0f; 
			is_action_locked = true;
			if (sfx_wizard_summon) sfx_wizard_summon->play();
			
			if (sprite) { 
				sprite->stop(); 
				last_attack_frame = -1; 
			}
			UtilityFunctions::print("[DEBUG-WIZARD] Summoning Block!");
		} 
		else if (key_e_just_pressed && has_teleport && wizard_teleport_cooldown_val <= 0.0f) {
			is_teleporting = true;
			wizard_action_timer = 1.4f; 
			wizard_teleport_cooldown_val = 2.0f; 
			is_action_locked = true;
			if (sfx_wizard_teleport) sfx_wizard_teleport->play();
			
			if (sprite) { 
				sprite->stop(); 
				last_attack_frame = -1; 
			}
		}
	}

	bool is_attacking = false;
	if (attack_timer > 0.0f) {
		is_attacking = true; 
	} 
	else if (mouse_left_just_pressed && !is_action_locked) {
		if (current_hero == KNIGHT && has_sword) {
			is_attacking = true;
			attack_timer = knight_attack_duration; 
			if (sfx_attack) sfx_attack->play();
			is_action_locked = true;
			
			if (sprite) {
				sprite->stop();
				last_attack_frame = -1;
			}
		} else if (current_hero == ARCHER && has_bow) {
			is_attacking = true;
			attack_timer = archer_attack_duration;
			if (sfx_archer_attack) sfx_archer_attack->play(); 
			is_action_locked = true;
			
			if (sprite) {
				sprite->stop();
				last_attack_frame = -1;
			}
		}
	}
	
	self->set_meta("attack_timer", attack_timer);

	if (!self->is_on_floor()) velocity.y += gravity * (float)delta;

	if (input->is_action_just_pressed("ui_accept") && self->is_on_floor()) {
		velocity.y = jump_velocity; 
		if (is_action_locked) {
			attack_timer = 0.0f;
			self->set_meta("attack_timer", 0.0f);
			
			is_attacking = false;
			is_blocking = false;
			is_healing = false;
			is_buffing = false;
			is_summoning = false;
			is_teleporting = false;
			
			block_timer = 0.0f;
			priest_action_timer = 0.0f;
			wizard_action_timer = 0.0f;
			is_action_locked = false; 
		}
	}

	float direction = input->get_axis("ui_left", "ui_right");
	
	if (is_action_locked) {
		if (self->is_on_floor()) {
			velocity.x = UtilityFunctions::move_toward(velocity.x, 0, speed * 6.0f * (float)delta); 
		}
	} 
	else {
		if (direction != 0) {
			velocity.x = direction * speed;
			if (sprite) sprite->set_flip_h(direction < 0);
		} else {
			velocity.x = UtilityFunctions::move_toward(velocity.x, 0, speed);
		}
	}

	if (self->is_on_floor() && velocity.x != 0 && !is_action_locked) {
		if (sfx_walk && !sfx_walk->is_playing()) {
			sfx_walk->play(); 
		}
	} else {
		if (sfx_walk && sfx_walk->is_playing()) {
			sfx_walk->stop(); 
		}
	}

	if (current_hero == ARCHER && has_grapple && input->is_action_just_pressed("grapple_hook") && !is_action_locked) {
		TypedArray<Node> targets = self->get_tree()->get_nodes_in_group("grapple_target");
		Node2D* nearest_valid_hook = nullptr;
		float shortest_distance = max_grapple_range; 

		for (int i = 0; i < targets.size(); i++) {
			Node2D* hook = Object::cast_to<Node2D>(targets[i]);
			if (hook) {
				float current_distance = self->get_global_position().distance_to(hook->get_global_position() + hook_visual_offset);
				if (current_distance < shortest_distance) {
					shortest_distance = current_distance;
					nearest_valid_hook = hook;
				}
			}
		}

		if (nearest_valid_hook) {
			is_grappling = true;
			grapple_launch_timer = grapple_launch_duration; 
			grapple_jump_timer = 0.0f; 
			grapple_target_pos = nearest_valid_hook->get_global_position() + hook_visual_offset; 
			
			if (sfx_archer_attack) sfx_archer_attack->play();
			
			if (sprite) {
				bool target_is_on_left = (grapple_target_pos.x < self->get_global_position().x);
				sprite->set_flip_h(target_is_on_left);
				sprite->play("archer_grapple_launch"); 
			}
		}
	}

	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : 
						(current_hero == ARCHER) ? "archer_" : 
						(current_hero == PRIEST) ? "priest_" : "wizard_";

		if (is_blocking) {
			sprite->play("knight_block");
		} else if (is_healing) {
			sprite->play("priest_heal");
		} else if (is_buffing) {
			sprite->play("priest_buff");
			
		} else if (is_summoning) {
			sprite->play("wizard_summon");
			int current_frame = sprite->get_frame();
			
			if (current_frame == 7 && last_attack_frame < 7) {
				Ref<PackedScene> block_scene = ResourceLoader::get_singleton()->load("res://scene/wizard_block.tscn");
				if (block_scene.is_valid()) {
					Node* block_inst = block_scene->instantiate();
					if (block_inst) {
						Node2D* block = Object::cast_to<Node2D>(block_inst);
						bool facing_right = !sprite->is_flipped_h();
						
						Vector2 spawn_offset = facing_right ? Vector2(50, -20) : Vector2(-50, -20);
						block->set_global_position(self->get_global_position() + spawn_offset);
						
						self->get_tree()->get_current_scene()->call_deferred(StringName("add_child"), block_inst);
						
						// Create 12 second timer to destroy block
						self->get_tree()->create_timer(12.0)->connect("timeout", Callable((Object*)block_inst, StringName("queue_free")));
					}
				}
			}
			last_attack_frame = current_frame;
			
		} else if (is_teleporting) {
			sprite->play("wizard_teleport");
			int current_frame = sprite->get_frame();
			
			if (current_frame == 3 && last_attack_frame < 3) {
				bool facing_right = !sprite->is_flipped_h();
				
				// Set your tile/block size here (assuming 16 or 32 pixels per block)
				float tile_size = 16.0f; 
				float teleport_distance = 7.0f * tile_size; 
				float teleport_x = facing_right ? teleport_distance : -teleport_distance;
				
				Vector2 new_pos = self->get_global_position();
				new_pos.x += teleport_x;
				self->set_global_position(new_pos);
				
				UtilityFunctions::print(String("[DEBUG-WIZARD] Facing Right: ") + (facing_right ? String("YES") : String("NO")) + String(" | TP X offset: ") + String::num(teleport_x));
			}
			last_attack_frame = current_frame;
			
		} else if (is_attacking) {
			sprite->play(prefix + "attack");
			
			int current_frame = sprite->get_frame();
			int damage_frame = (current_hero == KNIGHT) ? 4 : 9; 

			if (current_frame >= damage_frame && last_attack_frame < damage_frame) {
				bool facing_right = !sprite->is_flipped_h();

				if (current_hero == KNIGHT) {
					TypedArray<Node> enemies_grp = self->get_tree()->get_nodes_in_group("enemy");
					bool hit_anything = false;
					
					for (int i = 0; i < enemies_grp.size(); i++) {
						Node2D* enemy = Object::cast_to<Node2D>(enemies_grp[i]);
						if (enemy) {
							float dist = self->get_global_position().distance_to(enemy->get_global_position());
							Vector2 dir_to_enemy = enemy->get_global_position() - self->get_global_position();
							
							// Check if the enemy is actually in front of the player
							bool is_in_front = facing_right ? (dir_to_enemy.x > 0) : (dir_to_enemy.x < 0);

							// Reduced range from 95.0f to 45.0f AND added the directional check
							if (dist < 45.0f && is_in_front) {
								enemy->set_meta("pending_damage", knight_damage); 
								hit_anything = true;
								UtilityFunctions::print(String("[DEBUG-PLAYER] Knight hit enemy! Dist: ") + String::num(dist) + String(" | DMG: ") + String::num_int64(knight_damage));
							}
						}
					}
					if (!hit_anything) {
						UtilityFunctions::print("[DEBUG-PLAYER] Knight swung but missed.");
					}
				} 
				else if (current_hero == ARCHER) {
					UtilityFunctions::print("[DEBUG-ARROW] Attempting to spawn arrow...");
					
					Ref<PackedScene> arrow_scene = ResourceLoader::get_singleton()->load("res://scene/arrow.tscn");
					if (arrow_scene.is_valid()) {
						Node* arrow_instance = arrow_scene->instantiate();
						if (arrow_instance) {
							Node2D* arrow = Object::cast_to<Node2D>(arrow_instance);
							if (arrow) {
								Vector2 spawn_offset = facing_right ? Vector2(25, 2) : Vector2(-25, 2);
								arrow->set_global_position(self->get_global_position() + spawn_offset);
								arrow->set_scale(Vector2(0.5f, 0.5f));
								
								arrow->set_meta("buffed_damage", buff_active_timer > 0.0f);
								
								if (!facing_right) {
									arrow->set_rotation(3.14159f); 
								} else {
									arrow->set_rotation(0.0f);
								}

								self->get_tree()->get_current_scene()->call_deferred(StringName("add_child"), arrow_instance);
							}
						}
					}
				}
			}
			last_attack_frame = current_frame;
		} else {
			last_attack_frame = -1;
			if (!self->is_on_floor()) sprite->play(prefix + "jump");
			else if (velocity.x != 0) sprite->play(prefix + "walk");
			else sprite->play(prefix + "idle"); 
		}
	}

	self->set_velocity(velocity);
	self->move_and_slide();

	for (int i = 0; i < self->get_slide_collision_count(); i++) {
		Ref<KinematicCollision2D> collision = self->get_slide_collision(i);
		if (collision.is_valid()) {
			Node* collider = Object::cast_to<Node>(collision->get_collider());
			if (collider && collider->is_class("RigidBody2D")) {
				RigidBody2D* rb = Object::cast_to<RigidBody2D>(collider);
				if (rb) {
					Vector2 normal = collision->get_normal();
					if (normal.x > 0.5f || normal.x < -0.5f) {
						float push_force = 150.0f; 
						rb->apply_central_impulse(Vector2(-normal.x * push_force, 0));
					}
				}
			}
		}
	}

	bool is_currently_on_floor = self->is_on_floor();
	
	if (!was_on_floor && is_currently_on_floor) {
		if (sfx_landing) {
			sfx_landing->play();
		}
	}
	
	was_on_floor = is_currently_on_floor;
}

JENOVA_SCRIPT_END
