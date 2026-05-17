/* Jenova C++ Node Base Script (Meteora - Master Player Script) */
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/sprite2d.hpp> 
#include <Godot/classes/input.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/scene_tree_timer.hpp>
#include <Godot/classes/engine.hpp>
#include <Godot/classes/resource_loader.hpp>
#include <Godot/classes/packed_scene.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
Vector2 start_pos;
int inventory_count = 0;
bool is_dead = false;

// --- Combat Variables ---
bool has_sword = false;
bool has_shield = false;
bool has_bow = false; 
bool is_blocking = false;
float block_timer = 0.0f;
float block_cooldown = 0.0f;
float BLOCK_DURATION = 2.0f;
int player_health = 50;
int last_attack_frame = -1;
int knight_damage = 10;

// Hero unlock flags
bool unlocked_knight = true;
bool unlocked_archer = false;
bool unlocked_priest = false;

enum HeroType { KNIGHT = 0, ARCHER = 1, PRIEST = 2 };
HeroType current_hero = KNIGHT;

float speed = 70.0f;
float jump_velocity = -253.0f;
float gravity = 980.0f;

// ==========================================
// UNLOCK CONFIGURATION ROUTINES
// ==========================================
void unlock_sword(Caller* instance) { 
	has_sword = true; 
	Engine::get_singleton()->set_meta("save_has_sword", true);
	UtilityFunctions::print("[COMBAT MONITOR] Player unlocked Sword!"); 
}

void unlock_shield(Caller* instance) { 
	has_shield = true; 
	Engine::get_singleton()->set_meta("save_has_shield", true);
	UtilityFunctions::print("[COMBAT MONITOR] Player unlocked Shield!"); 
}

void unlock_bow(Caller* instance) { 
	has_bow = true; 
	Engine::get_singleton()->set_meta("save_has_bow", true);
	UtilityFunctions::print("[COMBAT MONITOR] Player unlocked Bow!"); 
}

void actually_teleport(Caller* instance);

void respawn() {
	if (is_dead) return;
	is_dead = true;
	UtilityFunctions::print("[COMBAT MONITOR] Player health dropped to 0! Respawning...");
	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : (current_hero == ARCHER) ? "archer_" : "priest_";
		sprite->play(prefix + "death");
	}
	if (self) self->set_velocity(Vector2(0, 0));
	self->get_tree()->create_timer(1.0)->connect("timeout", Callable((Object*)self, "actually_teleport"));
}

void take_damage(int amount) {
	if (is_dead) return;
	
	if (is_blocking && current_hero == KNIGHT) {
		UtilityFunctions::print("[COMBAT MONITOR] SHIELD BLOCKED! Damage completely absorbed.");
		return;
	}
	
	player_health -= amount;
	UtilityFunctions::print("[COMBAT MONITOR] Player hit! HP Left: ", player_health);
	
	if (player_health <= 0) respawn();
}

int increase_inventory(Caller* instance) { inventory_count += 1; return inventory_count; }
int get_inventory_count(Caller* instance) { return inventory_count; }

void actually_teleport(Caller* instance) {
	CharacterBody2D* player = GetSelf<CharacterBody2D>(instance);
	if (player) {
		player->set_global_position(start_pos);
		is_dead = false;
		player_health = 50;
		is_blocking = false;
		block_cooldown = 0.0f;
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
		// FIXED: Collectibles counter explicitly reset to 0 upon every fresh level lifecycle load
		inventory_count = 0; 

		start_pos = self->get_global_position();
		is_dead = false;
		player_health = 50;

		Engine* engine = Engine::get_singleton();
		has_sword = engine->has_meta("save_has_sword") ? (bool)engine->get_meta("save_has_sword") : false;
		has_shield = engine->has_meta("save_has_shield") ? (bool)engine->get_meta("save_has_shield") : false;
		has_bow = engine->has_meta("save_has_bow") ? (bool)engine->get_meta("save_has_bow") : false;
		
		String scene_name = self->get_tree()->get_current_scene()->get_name();
		
		if (scene_name == "level2" || scene_name == "Level2") {
			unlocked_archer = true; 
			if (!has_sword) {
				UtilityFunctions::print("DEV CHEAT: Giving testing gear layout.");
				has_sword = true;
				has_shield = true;
			}
		} else {
			unlocked_archer = false; 
		}

		unlocked_priest = false; 
		unlocked_knight = true;
		current_hero = KNIGHT;
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || is_dead) return;

	Input* input = Input::get_singleton();
	Vector2 velocity = self->get_velocity();

	if (input->is_action_just_pressed("hero_1") && unlocked_knight) current_hero = KNIGHT;
	if (input->is_action_just_pressed("hero_2") && unlocked_archer) current_hero = ARCHER;
	if (input->is_action_just_pressed("hero_3") && unlocked_priest) current_hero = PRIEST;

	if (!self->is_on_floor()) velocity.y += gravity * (float)delta;
	if (input->is_action_just_pressed("ui_accept") && self->is_on_floor()) velocity.y = jump_velocity;

	float direction = input->get_axis("ui_left", "ui_right");
	if (direction != 0) {
		velocity.x = direction * speed;
		if (sprite) sprite->set_flip_h(direction < 0);
	} else {
		velocity.x = UtilityFunctions::move_toward(velocity.x, 0, speed);
	}

	if (current_hero == KNIGHT && has_shield) {
		if (block_cooldown > 0.0f) block_cooldown -= (float)delta;
		if (input->is_key_pressed(Key::KEY_Q) && !is_blocking && block_cooldown <= 0.0f) {
			is_blocking = true;
			block_timer = BLOCK_DURATION;
			UtilityFunctions::print("[COMBAT MONITOR] Shield Up!");
		}
		if (is_blocking) {
			block_timer -= (float)delta;
			if (block_timer <= 0.0f) {
				is_blocking = false;
				block_cooldown = 2.0f;
				UtilityFunctions::print("[COMBAT MONITOR] Shield Down (Cooldown initialized).");
			}
		}
	} else {
		is_blocking = false;
	}

	bool is_attacking = false;
	if (input->is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) && !is_blocking) {
		if (current_hero == KNIGHT && has_sword) is_attacking = true;
		if (current_hero == ARCHER && has_bow) is_attacking = true; 
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
					TypedArray<Node> enemies = self->get_tree()->get_nodes_in_group("enemy");
					for (int i = 0; i < enemies.size(); i++) {
						Node2D* enemy = Object::cast_to<Node2D>(enemies[i]);
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
							
							if (!facing_right) {
								arrow->set_scale(Vector2(-0.5f, 0.5f)); 
							} else {
								arrow->set_scale(Vector2(0.5f, 0.5f));  
							}

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
