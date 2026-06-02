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
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/string_name.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
ProgressBar* health_bar = nullptr;

// --- Death Screen Pointers ---
Control* death_screen = nullptr;

// --- Audio Pointers ---
AudioStreamPlayer* bg_music = nullptr;
AudioStreamPlayer* combat_music = nullptr;
AudioStreamPlayer* sfx_walk = nullptr;
AudioStreamPlayer* sfx_attack = nullptr;
AudioStreamPlayer* sfx_block = nullptr;
float combat_cooldown = 0.0f; 

Vector2 start_pos;
int inventory_count = 0;
bool is_dead = false;

// --- Combat Configurations ---
bool has_sword = false;
bool has_shield = false;
bool has_bow = false;  

float knight_attack_duration = 0.66f; 
float archer_attack_duration = 1.0f; 

bool is_blocking = false;
float block_timer = 0.0f;
float block_cooldown_val = 0.0f;
float BLOCK_DURATION = 2.0f;
int player_health = 50;
int last_attack_frame = -1;
int knight_damage = 10;

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

enum HeroType { KNIGHT = 0, ARCHER = 1, PRIEST = 2 };
HeroType current_hero = KNIGHT;

float speed = 70.0f;
float jump_velocity = -253.0f;
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
	
	UtilityFunctions::print("[DEBUG-DEATH] Player died. Triggering respawn logic.");

	Line2D* rope = Object::cast_to<Line2D>(self->get_node_or_null("GrappleLine"));
	if (rope) rope->set_visible(false);

	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : (current_hero == ARCHER) ? "archer_" : "priest_";
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
			return true; 
		}
		return false; 
	}
	
	player_health -= amount;
	if (health_bar) health_bar->set_value((double)player_health);
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
		if (health_bar) health_bar->set_value(50.0);
		is_blocking = false;
		block_cooldown_val = 0.0f;

		if (death_screen) death_screen->set_visible(false);
		if (sprite) {
			String prefix = (current_hero == KNIGHT) ? "knight_" : (current_hero == ARCHER) ? "archer_" : "priest_";
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
		self->set_meta("attack_timer", 0.0f);
		
		player_health = 50;
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
					UtilityFunctions::print("[DEBUG-DEATH] Respawn Button successfully connected.");
				}
			} else {
				UtilityFunctions::print("[DEBUG-DEATH] ERROR: Death screen found, but RespawnButton is missing!");
			}
		}

		bg_music = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("LevelBGM"));
		combat_music = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("CombatBGM"));
		sfx_walk = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("WalkSFX"));
		sfx_attack = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("AttackSFX"));
		sfx_block = Object::cast_to<AudioStreamPlayer>(self->get_node_or_null("BlockSFX"));

		if (bg_music) {
			bg_music->set_volume_db(0.0f);
			bg_music->play();
		}
		if (combat_music) {
			combat_music->set_volume_db(-60.0f); 
			combat_music->play();
		}

		Engine* engine = Engine::get_singleton();
		has_sword = engine->has_meta("save_has_sword") ? (bool)engine->get_meta("save_has_sword") : false;
		has_shield = engine->has_meta("save_has_shield") ? (bool)engine->get_meta("save_has_shield") : false;
		has_bow = engine->has_meta("save_has_bow") ? (bool)engine->get_meta("save_has_bow") : false;
		has_grapple = engine->has_meta("save_has_grapple") ? (bool)engine->get_meta("save_has_grapple") : false;
		
		String scene_name = self->get_tree()->get_current_scene()->get_name();
		if (scene_name == "level2" || scene_name == "Level2") {
			unlocked_archer = true; 
			has_sword = true;
			has_shield = true;
		} else {
			unlocked_archer = false; 
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

	float attack_timer = self->has_meta("attack_timer") ? (float)self->get_meta("attack_timer") : 0.0f;

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
		if (input->is_action_just_pressed("hero_2") && unlocked_archer) {
			current_hero = ARCHER;
			is_blocking = false;
		}
		if (input->is_action_just_pressed("hero_3") && unlocked_priest) current_hero = PRIEST;
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

	if (current_hero == KNIGHT && has_shield) {
		if (block_cooldown_val > 0.0f) block_cooldown_val -= (float)delta;
		if (key_q_just_pressed && !is_blocking && block_cooldown_val <= 0.0f && attack_timer <= 0.0f && self->is_on_floor()) {
			is_blocking = true;
			block_timer = BLOCK_DURATION;
			if (sfx_block) sfx_block->play();
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

	bool is_attacking = false;
	if (attack_timer > 0.0f) {
		is_attacking = true; 
	} 
	else if (mouse_left_just_pressed && !is_blocking) {
		if (current_hero == KNIGHT && has_sword) {
			is_attacking = true;
			attack_timer = knight_attack_duration; 
			if (sfx_attack) sfx_attack->play();
		} else if (current_hero == ARCHER && has_bow) {
			is_attacking = true;
			attack_timer = archer_attack_duration; 
			if (sfx_attack) sfx_attack->play();
		}
	}
	
	self->set_meta("attack_timer", attack_timer);
	bool is_action_locked = is_attacking || is_blocking;

	if (!self->is_on_floor()) velocity.y += gravity * (float)delta;

	if (input->is_action_just_pressed("ui_accept") && self->is_on_floor()) {
		velocity.y = jump_velocity; 
		if (is_action_locked) {
			attack_timer = 0.0f;
			self->set_meta("attack_timer", 0.0f);
			is_attacking = false;
			is_blocking = false;
			block_timer = 0.0f;
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
			
			if (sprite) {
				bool target_is_on_left = (grapple_target_pos.x < self->get_global_position().x);
				sprite->set_flip_h(target_is_on_left);
				sprite->play("archer_grapple_launch"); 
			}
		}
	}

	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : (current_hero == ARCHER) ? "archer_" : "priest_";

		if (is_blocking) {
			sprite->play("knight_block");
		} else if (is_attacking) {
			sprite->play(prefix + "attack");
			int current_frame = sprite->get_frame();
			int damage_frame = (current_hero == KNIGHT) ? 4 : 10;

			if (current_frame == damage_frame && last_attack_frame != damage_frame) {
				bool facing_right = !sprite->is_flipped_h();

				if (current_hero == KNIGHT) {
					TypedArray<Node> enemies_grp = self->get_tree()->get_nodes_in_group("enemy");
					for (int i = 0; i < enemies_grp.size(); i++) {
						Node2D* enemy = Object::cast_to<Node2D>(enemies_grp[i]);
						if (enemy && self->get_global_position().distance_to(enemy->get_global_position()) < 60.0f) {
							enemy->set_meta("pending_damage", knight_damage);
						}
					}
				} 
				else if (current_hero == ARCHER) {
					Ref<PackedScene> arrow_scene = ResourceLoader::get_singleton()->load("res://scene/arrow.tscn");
					if (arrow_scene.is_valid()) {
						Node* arrow_instance = arrow_scene->instantiate();
						Node2D* arrow = Object::cast_to<Node2D>(arrow_instance);
						if (arrow) {
							Vector2 spawn_offset = facing_right ? Vector2(20, 2) : Vector2(-20, 2);
							arrow->set_global_position(self->get_global_position() + spawn_offset);
							arrow->set_scale(facing_right ? Vector2(0.5f, 0.5f) : Vector2(-0.5f, 0.5f));
							self->get_tree()->get_current_scene()->add_child(arrow_instance);
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
}

JENOVA_SCRIPT_END
