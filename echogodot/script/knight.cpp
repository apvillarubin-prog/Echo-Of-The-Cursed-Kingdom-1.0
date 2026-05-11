/* Jenova C++ Node Base Script (Meteora - Fully Merged) */
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/scene_tree_timer.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

CharacterBody2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
Vector2 start_pos;
int inventory_count = 0;
bool is_dead = false;

bool has_sword = false;
bool has_shield = false;
bool is_blocking = false;
float block_timer = 0.0f;
float block_cooldown = 0.0f;
float BLOCK_DURATION = 2.0f;
int player_health = 50;
int last_attack_frame = -1;

enum HeroType { KNIGHT = 0, ARCHER = 1 };
HeroType current_hero = KNIGHT;

float speed = 70.0f;
float jump_velocity = -253.0f;
float gravity = 980.0f;

void unlock_sword() {
	if (current_hero == KNIGHT) {
		has_sword = true;
		UtilityFunctions::print("Sword Unlocked for the Knight!");
	} else {
		UtilityFunctions::print("Archer found a sword, but doesn't know how to use it.");
	}
}

void unlock_shield() {
	if (current_hero == KNIGHT) {
		has_shield = true;
		UtilityFunctions::print("Shield Unlocked for the Knight!");
	} else {
		UtilityFunctions::print("Archer found a shield, but doesn't know how to use it.");
	}
}

void actually_teleport(Caller* instance);

void respawn() {
	if (is_dead) return;
	is_dead = true;
	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : "archer_";
		sprite->play(prefix + "death");
	}
	if (self) self->set_velocity(Vector2(0, 0));
	self->get_tree()->create_timer(1.0)->connect("timeout", Callable((Object*)self, "actually_teleport"));
}

void take_damage(int amount) {
	if (is_dead) return;
	if (is_blocking) {
		UtilityFunctions::print("DEBUG [Knight] Blocked! Damage ignored.");
		return;
	}
	player_health -= amount;
	UtilityFunctions::print("Player Health: ", player_health);
	if (player_health <= 0) {
		respawn();
	}
}

void increase_inventory(Caller* instance) {
	inventory_count += 1;
	UtilityFunctions::print("Item Collected! Total: ", inventory_count);
}

void actually_teleport(Caller* instance) {
	CharacterBody2D* player = GetSelf<CharacterBody2D>(instance);
	if (player) {
		player->set_global_position(start_pos);
		is_dead = false;
		player_health = 50;
		is_blocking = false;
		block_timer = 0.0f;
		block_cooldown = 0.0f;
		UtilityFunctions::print("DEBUG [Knight] Respawned. Block state reset.");
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
		start_pos = self->get_global_position();
		is_dead = false;
		player_health = 50;
		is_blocking = false;
		block_timer = 0.0f;
		block_cooldown = 0.0f;
		UtilityFunctions::print("Hero script active. Inventory reset.");
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || is_dead) return;

	Input* input = Input::get_singleton();

	if (input->is_action_just_pressed("hero_1")) current_hero = KNIGHT;
	else if (input->is_action_just_pressed("hero_2")) current_hero = ARCHER;

	Vector2 velocity = self->get_velocity();

	if (!self->is_on_floor()) {
		velocity.y += gravity * (float)delta;
	}

	if (input->is_action_just_pressed("ui_accept") && self->is_on_floor()) {
		velocity.y = jump_velocity;
	}

	float direction = input->get_axis("ui_left", "ui_right");
	if (direction != 0) {
		velocity.x = direction * speed;
		if (sprite) sprite->set_flip_h(direction < 0);
	} else {
		velocity.x = UtilityFunctions::move_toward(velocity.x, 0, speed);
	}

	
	if (current_hero == KNIGHT && has_shield) {
		
		if (block_cooldown > 0.0f) {
			block_cooldown -= (float)delta;
			if (block_cooldown < 0.0f) block_cooldown = 0.0f;
		}

		
		if (input->is_key_pressed(Key::KEY_Q) && !is_blocking && block_cooldown <= 0.0f) {
			is_blocking = true;
			block_timer = BLOCK_DURATION;
			UtilityFunctions::print("DEBUG [Knight] Block started! Duration: ", BLOCK_DURATION, "s");
		}

		
		if (input->is_key_pressed(Key::KEY_Q) && !is_blocking && block_cooldown > 0.0f) {
			UtilityFunctions::print("DEBUG [Knight] Block on cooldown! Remaining: ", block_cooldown, "s");
		}

		if (is_blocking) {
			block_timer -= (float)delta;
			if (block_timer <= 0.0f) {
				is_blocking = false;
				block_timer = 0.0f;
				block_cooldown = 2.0f;
				UtilityFunctions::print("DEBUG [Knight] Block ended. Cooldown started: 5s");
			}
		}
	}

	
	bool is_attacking = false;
	if (current_hero == KNIGHT && has_sword && !is_blocking && input->is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT)) {
		is_attacking = true;
	}

	if (sprite) {
		String prefix = (current_hero == KNIGHT) ? "knight_" : "archer_";

		if (is_blocking) {
			sprite->play("knight_block");
		} else if (is_attacking) {
			sprite->play(prefix + "attack");
			int current_frame = sprite->get_frame();

			if (current_frame == 4 && last_attack_frame != 4) {
				TypedArray<Node> enemies = self->get_tree()->get_nodes_in_group("enemy");
				for (int i = 0; i < enemies.size(); i++) {
					Object* obj = enemies[i];
					if (!obj) continue;
					Node* enemy_node = Object::cast_to<Node>(obj);
					if (!enemy_node) continue;
					if (!ObjectDB::get_instance(enemy_node->get_instance_id())) continue;

					Node2D* enemy = Object::cast_to<Node2D>(enemy_node);
					if (enemy) {
						if (self->get_global_position().distance_to(enemy->get_global_position()) < 60.0f) {
							enemy->call("take_hit");
						}
					}
				}
			}
			last_attack_frame = current_frame;
		} else {
			last_attack_frame = -1;
			if (!self->is_on_floor()) sprite->play(prefix + "jump");
			else if (direction != 0) sprite->play(prefix + "walk");
			else sprite->play(prefix + "idle");
		}
	}

	self->set_velocity(velocity);
	self->move_and_slide();
}

JENOVA_SCRIPT_END
