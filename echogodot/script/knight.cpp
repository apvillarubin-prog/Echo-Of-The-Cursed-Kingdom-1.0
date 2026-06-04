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
#include <Godot/classes/audio_stream_player.hpp> 
#include <Godot/classes/kinematic_collision2d.hpp>
#include <Godot/classes/rigid_body2d.hpp>          
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/string_name.hpp>
#include <Godot/variant/color.hpp>
#include <unordered_map>
#include <memory>

using namespace godot;
using namespace jenova::sdk;

class PlayerController; 

class BaseHero {
protected:
	PlayerController* core;
public:
	BaseHero(PlayerController* p) : core(p) {}
	virtual ~BaseHero() = default;

	virtual void update(double delta, Input* input, Vector2& velocity) = 0;
	virtual bool on_take_damage(int& amount) { return false; } 
	virtual bool is_action_locked() { return false; }
};

class PlayerController {
public:
	CharacterBody2D* self = nullptr;
	AnimatedSprite2D* sprite = nullptr;

	std::unique_ptr<BaseHero> active_hero;
	bool admin_mode = true; 
	bool is_dead = false;
	int player_health = 50;
	Vector2 start_pos;
	int inventory_count = 0;
	float damage_flash_timer = 0.0f;
	bool was_on_floor = true; 

	float speed = 70.0f;
	float jump_velocity = -253.0f;
	float gravity = 980.0f;

	bool mouse_left_just_pressed = false, prev_mouse = false;
	bool key_q_just_pressed = false, prev_key_q = false;
	bool key_e_just_pressed = false, prev_key_e = false;
	bool key_f_just_pressed = false, prev_key_f = false;
	bool key_g_just_pressed = false, prev_key_g = false;

	float buff_active_timer = 0.0f;
	float combat_cooldown = 0.0f; 

	AudioStreamPlayer *bg_music = nullptr, *combat_music = nullptr, *sfx_walk = nullptr, *sfx_attack = nullptr;
	AudioStreamPlayer *sfx_block = nullptr, *sfx_landing = nullptr, *sfx_defeat = nullptr, *sfx_archer_attack = nullptr;
	AudioStreamPlayer *sfx_priest_heal = nullptr, *sfx_priest_buff = nullptr, *sfx_wizard_summon = nullptr, *sfx_wizard_teleport = nullptr;

	bool unlocked_knight = true, unlocked_archer = false, unlocked_priest = false, unlocked_wizard = false;
	bool has_sword = false, has_shield = false, has_bow = false, has_grapple = false;
	bool has_heal = false, has_buff = false, has_summon = false, has_teleport = false;
	bool has_mouse_teleport = false, has_ice = false, has_fireball = false;

	void play_sfx(AudioStreamPlayer* sfx) { if (sfx) sfx->play(); }
	void stop_sfx(AudioStreamPlayer* sfx) { if (sfx && sfx->is_playing()) sfx->stop(); }

	void switch_hero(int type); 

	bool take_damage(int amount) {
		if (is_dead) return false;
		
		if (active_hero && active_hero->on_take_damage(amount)) return true; 
		
		player_health -= amount;
		damage_flash_timer = 0.15f; 
		if (sprite) sprite->set_modulate(Color(5.0f, 0.3f, 0.3f, 1.0f));

		Node* game_ui = self->get_tree()->get_first_node_in_group("game_ui");
		if (game_ui) game_ui->call("update_health", player_health);
		
		if (player_health <= 0) respawn();
		return false; 
	}

	void respawn() {
		if (is_dead) return;
		is_dead = true;
		if (sfx_defeat) sfx_defeat->play();

		Line2D* rope = Object::cast_to<Line2D>(self->get_node_or_null("GrappleLine"));
		if (rope) rope->set_visible(false);
		
		if (sprite) {
			String prefix = (unlocked_wizard && active_hero) ? "wizard_" : "knight_"; 
			sprite->play(prefix + "death");
			sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f)); 
		}
		
		self->set_velocity(Vector2(0, 0));
		
		Node* game_ui = self->get_tree()->get_first_node_in_group("game_ui");
		if (game_ui) {
			game_ui->call("show_death_screen");
		} else {
			UtilityFunctions::print("[Engine] WARNING: UI Group not found! Falling back to 1-sec auto-respawn.");
			self->get_tree()->create_timer(1.0)->connect("timeout", Callable((Object*)self, "actually_teleport"));
		}
	}

	void actually_teleport() {
		if (self) {
			self->set_global_position(start_pos);
			is_dead = false;
			player_health = 50;
			
			Node* game_ui = self->get_tree()->get_first_node_in_group("game_ui");
			if (game_ui) game_ui->call("update_health", 50);

			switch_hero(0); 
			if (sprite) sprite->play("knight_idle");
		}
	}

	void on_ready(CharacterBody2D* node);
	void on_physics_process(double delta);
};


class Knight : public BaseHero {
private:
	float attack_timer = 0.0f, block_timer = 0.0f, block_cooldown = 0.0f;
	bool is_blocking = false;
	int last_attack_frame = -1;

public:
	Knight(PlayerController* p) : BaseHero(p) {}

	bool is_action_locked() override { return (attack_timer > 0.0f) || is_blocking; }

	bool on_take_damage(int& amount) override {
		if (is_blocking) {
			if (block_timer > 1.5f - 0.3f) return true; 
			amount /= 2; 
		}
		return false;
	}

	void update(double delta, Input* input, Vector2& velocity) override {
		if (attack_timer > 0.0f) attack_timer -= (float)delta;
		if (block_cooldown > 0.0f) block_cooldown -= (float)delta;
		
		if (is_blocking) {
			block_timer -= (float)delta;
			if (block_timer <= 0.0f) { is_blocking = false; block_cooldown = 1.0f; }
		}

		if (core->has_shield && core->key_q_just_pressed && !is_blocking && block_cooldown <= 0.0f && attack_timer <= 0.0f && core->self->is_on_floor()) {
			is_blocking = true; block_timer = 1.5f;
			core->play_sfx(core->sfx_block);
		}
		else if (core->has_sword && core->mouse_left_just_pressed && !is_action_locked()) {
			attack_timer = 0.66f;
			core->play_sfx(core->sfx_attack);
			if (core->sprite) { core->sprite->stop(); last_attack_frame = -1; }
		}

		float direction = input->get_axis("ui_left", "ui_right");
		if (is_action_locked()) {
			if (core->self->is_on_floor()) velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed * 6.0f * (float)delta); 
		} else {
			if (direction != 0) {
				velocity.x = direction * core->speed;
				if (core->sprite) core->sprite->set_flip_h(direction < 0);
			} else {
				velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed);
			}
		}

		if (core->sprite) {
			if (is_blocking) {
				core->sprite->play("knight_block");
			} else if (attack_timer > 0.0f) {
				core->sprite->play("knight_attack");
				int current_frame = core->sprite->get_frame();
				if (current_frame >= 4 && last_attack_frame < 4) {
					bool facing_right = !core->sprite->is_flipped_h();
					TypedArray<Node> enemies = core->self->get_tree()->get_nodes_in_group("enemy");
					for (int i = 0; i < enemies.size(); i++) {
						Node2D* enemy = Object::cast_to<Node2D>(enemies[i]);
						if (enemy) {
							Vector2 diff = enemy->get_global_position() - core->self->get_global_position();
							bool in_front = facing_right ? (diff.x > 0) : (diff.x < 0);
							if (diff.length() < 45.0f && in_front) enemy->set_meta("pending_damage", (core->buff_active_timer > 0.0f) ? 20 : 10); 
						}
					}
				}
				last_attack_frame = current_frame;
			} else {
				last_attack_frame = -1;
				if (!core->self->is_on_floor()) core->sprite->play("knight_jump");
				else if (velocity.x != 0) core->sprite->play("knight_walk");
				else core->sprite->play("knight_idle"); 
			}
		}
		if (core->self->is_on_floor() && velocity.x != 0 && !is_action_locked()) core->play_sfx(core->sfx_walk); 
		else core->stop_sfx(core->sfx_walk);
	}
};

class Archer : public BaseHero {
private:
	float attack_timer = 0.0f;
	int last_attack_frame = -1;
	
	bool is_grappling = false;
	float grapple_launch_timer = 0.0f, grapple_jump_timer = 0.0f;
	Vector2 grapple_target_pos;
	float grapple_radius = 0.0f, max_grapple_range = 160.0f;

public:
	Archer(PlayerController* p) : BaseHero(p) {}

	bool is_action_locked() override { return (attack_timer > 0.0f) || is_grappling; }

	void update(double delta, Input* input, Vector2& velocity) override {
		if (attack_timer > 0.0f) attack_timer -= (float)delta;
		
		Line2D* grapple_line = Object::cast_to<Line2D>(core->self->get_node_or_null("GrappleLine"));
		Node2D* hand_anchor = Object::cast_to<Node2D>(core->self->get_node_or_null("BowHandAnchor"));

		if (is_grappling) {
			if (input->is_action_just_pressed("grapple_hook") || input->is_action_just_pressed("ui_accept")) {
				is_grappling = false;
				if (grapple_line) grapple_line->set_visible(false);
				if (input->is_action_just_pressed("ui_accept")) velocity.y = core->jump_velocity;
				return;
			}
			if (grapple_launch_timer > 0.0f) {
				grapple_launch_timer -= (float)delta;
				velocity = Vector2(0, 0); 
				if (core->sprite) core->sprite->play("archer_grapple_launch");
				if (grapple_line) grapple_line->set_visible(false);
				if (grapple_launch_timer <= 0.0f) {
					grapple_jump_timer = 0.35f;
					Vector2 current_pos = core->self->get_global_position();
					current_pos.y -= 18.0f; 
					core->self->set_global_position(current_pos);
					if (core->sprite) core->sprite->play("archer_jump");
					
					velocity = (grapple_target_pos - core->self->get_global_position()).normalized() * 260.0f; 
					if (velocity.y > -140.0f) velocity.y = -180.0f; 
				}
			} 
			else if (grapple_jump_timer > 0.0f) {
				grapple_jump_timer -= (float)delta;
				if (core->sprite) core->sprite->play("archer_jump");
				velocity.y += core->gravity * (float)delta;
				if (grapple_jump_timer <= 0.0f) grapple_radius = std::max(30.0f, (float)(core->self->get_global_position() - grapple_target_pos).length());
			}
			else {
				float climb_dir = (input->is_action_pressed("ui_up") || input->is_key_pressed(Key::KEY_W)) ? -1.0f : 
								  (input->is_action_pressed("ui_down") || input->is_key_pressed(Key::KEY_S)) ? 1.0f : 0.0f;
				if (climb_dir != 0.0f) {
					grapple_radius = UtilityFunctions::clamp(grapple_radius + climb_dir * 140.0f * (float)delta, 30.0f, max_grapple_range);
					if (core->sprite) core->sprite->play("archer_grapple_pull"); 
				} else {
					if (core->sprite) core->sprite->play("archer_swing"); 
				}

				velocity.y += core->gravity * (float)delta;
				Vector2 rope_dir = (core->self->get_global_position() - grapple_target_pos).normalized();
				Vector2 tangent = Vector2(-rope_dir.y, rope_dir.x).normalized();
				float swing_input = input->get_axis("ui_left", "ui_right");
				if (swing_input != 0.0f) {
					velocity += tangent * (-swing_input) * 280.0f * (float)delta; 
					if (core->sprite) core->sprite->set_flip_h(swing_input < 0);
				}
				velocity = tangent * (velocity.dot(tangent) * 0.990f) - rope_dir * ((core->self->get_global_position() - grapple_target_pos).length() - grapple_radius) * 25.0f;
			}
			
			if (grapple_launch_timer <= 0.0f && grapple_jump_timer <= 0.0f && (core->self->is_on_floor() || core->self->is_on_wall() || core->self->is_on_ceiling())) {
				is_grappling = false;
				if (grapple_line) grapple_line->set_visible(false);
			}

			if (is_grappling && grapple_launch_timer <= 0.0f && grapple_jump_timer <= 0.0f) {
				core->self->set_global_position(grapple_target_pos + (core->self->get_global_position() - grapple_target_pos).normalized() * grapple_radius);
			}
			if (is_grappling && grapple_launch_timer <= 0.0f && grapple_line && hand_anchor) {
				grapple_line->set_visible(true);
				if (grapple_line->get_point_count() < 2) { grapple_line->clear_points(); grapple_line->add_point(Vector2(0,0)); grapple_line->add_point(Vector2(0,0)); }
				grapple_line->set_point_position(0, grapple_line->to_local(hand_anchor->get_global_position()));
				grapple_line->set_point_position(1, grapple_line->to_local(grapple_target_pos));
			}
			return; 
		}

		if (core->has_grapple && input->is_action_just_pressed("grapple_hook") && !is_action_locked()) {
			TypedArray<Node> targets = core->self->get_tree()->get_nodes_in_group("grapple_target");
			Node2D* nearest = nullptr;
			float shortest = max_grapple_range; 
			Vector2 hook_visual_offset = Vector2(0, 8);
			for (int i = 0; i < targets.size(); i++) {
				Node2D* hook = Object::cast_to<Node2D>(targets[i]);
				if (hook && core->self->get_global_position().distance_to(hook->get_global_position() + hook_visual_offset) < shortest) {
					shortest = core->self->get_global_position().distance_to(hook->get_global_position() + hook_visual_offset); nearest = hook;
				}
			}
			if (nearest) {
				is_grappling = true; grapple_launch_timer = 0.65f; grapple_jump_timer = 0.0f; 
				grapple_target_pos = nearest->get_global_position() + hook_visual_offset; 
				core->play_sfx(core->sfx_archer_attack);
				if (core->sprite) {
					core->sprite->set_flip_h(grapple_target_pos.x < core->self->get_global_position().x);
					core->sprite->play("archer_grapple_launch"); 
				}
			}
		}
		else if (core->has_bow && core->mouse_left_just_pressed && !is_action_locked()) {
			attack_timer = 1.08f;
			core->play_sfx(core->sfx_archer_attack);
			if (core->sprite) { core->sprite->stop(); last_attack_frame = -1; }
		}

		float direction = input->get_axis("ui_left", "ui_right");
		if (is_action_locked()) {
			if (core->self->is_on_floor()) velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed * 6.0f * (float)delta); 
		} else {
			if (direction != 0) {
				velocity.x = direction * core->speed;
				if (core->sprite) core->sprite->set_flip_h(direction < 0);
			} else {
				velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed);
			}
		}

		if (core->sprite) {
			if (attack_timer > 0.0f) {
				core->sprite->play("archer_attack");
				int current_frame = core->sprite->get_frame();
				if (current_frame >= 9 && last_attack_frame < 9) {
					bool facing_right = !core->sprite->is_flipped_h();
					Ref<PackedScene> arrow_scene = ResourceLoader::get_singleton()->load("res://scene/arrow.tscn");
					if (arrow_scene.is_valid()) {
						Node* arrow_instance = arrow_scene->instantiate();
						if (arrow_instance) {
							core->self->get_tree()->get_current_scene()->add_child(arrow_instance);
							Object::cast_to<Node2D>(arrow_instance)->set_global_position(core->self->get_global_position() + (facing_right ? Vector2(25, -15) : Vector2(-25, -15)));
							Object::cast_to<Node2D>(arrow_instance)->set_scale(Vector2(0.5f, 0.5f));
							Object::cast_to<Node2D>(arrow_instance)->set_meta("buffed_damage", core->buff_active_timer > 0.0f);
							Object::cast_to<Node2D>(arrow_instance)->set_meta("facing_left", !facing_right);
						}
					}
				}
				last_attack_frame = current_frame;
			} else {
				last_attack_frame = -1;
				if (!core->self->is_on_floor()) core->sprite->play("archer_jump");
				else if (velocity.x != 0) core->sprite->play("archer_walk");
				else core->sprite->play("archer_idle"); 
			}
		}
		if (core->self->is_on_floor() && velocity.x != 0 && !is_action_locked()) core->play_sfx(core->sfx_walk); 
		else core->stop_sfx(core->sfx_walk);
	}
};


class Priest : public BaseHero {
private:
	float heal_cooldown = 0.0f, buff_cooldown = 0.0f, action_timer = 0.0f;
	bool is_healing = false, is_buffing = false;

public:
	Priest(PlayerController* p) : BaseHero(p) {}
	bool is_action_locked() override { return is_healing || is_buffing; }

	void update(double delta, Input* input, Vector2& velocity) override {
		if (heal_cooldown > 0.0f) heal_cooldown -= (float)delta;
		if (buff_cooldown > 0.0f) buff_cooldown -= (float)delta;
		if (action_timer > 0.0f) {
			action_timer -= (float)delta;
			if (action_timer <= 0.0f) { is_healing = false; is_buffing = false; }
		}

		if (!is_action_locked() && core->self->is_on_floor()) {
			if (core->key_e_just_pressed && core->has_heal && heal_cooldown <= 0.0f) {
				is_healing = true; action_timer = 1.2f; heal_cooldown = 3.0f;
				core->play_sfx(core->sfx_priest_heal);
				core->player_health = std::min(50, core->player_health + 10);
				Node* game_ui = core->self->get_tree()->get_first_node_in_group("game_ui");
				if (game_ui) game_ui->call("update_health", core->player_health);
			} 
			else if (core->key_q_just_pressed && core->has_buff && buff_cooldown <= 0.0f) {
				is_buffing = true; action_timer = 1.2f; buff_cooldown = 3.0f; core->buff_active_timer = 10.0f;
				core->play_sfx(core->sfx_priest_buff); 
			}
		}

		float direction = input->get_axis("ui_left", "ui_right");
		if (is_action_locked()) {
			if (core->self->is_on_floor()) velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed * 6.0f * (float)delta); 
		} else {
			if (direction != 0) {
				velocity.x = direction * core->speed;
				if (core->sprite) core->sprite->set_flip_h(direction < 0);
			} else {
				velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed);
			}
		}

		if (core->sprite) {
			if (is_healing) core->sprite->play("priest_heal");
			else if (is_buffing) core->sprite->play("priest_buff");
			else if (!core->self->is_on_floor()) core->sprite->play("priest_jump");
			else if (velocity.x != 0) core->sprite->play("priest_walk");
			else core->sprite->play("priest_idle"); 
		}
		if (core->self->is_on_floor() && velocity.x != 0 && !is_action_locked()) core->play_sfx(core->sfx_walk); 
		else core->stop_sfx(core->sfx_walk);
	}
};

class Wizard : public BaseHero {
private:
	float summon_cooldown = 0.0f, teleport_cooldown = 0.0f, q_cooldown = 0.0f, f_cooldown = 0.0f, g_cooldown = 0.0f, action_timer = 0.0f;
	int last_attack_frame = -1;
	bool is_summoning = false, is_teleporting = false, is_teleport_stance = false, is_f_primed = false, is_g_primed = false;

public:
	Wizard(PlayerController* p) : BaseHero(p) {}
	bool is_action_locked() override { return is_summoning || is_teleporting || is_teleport_stance; }

	void update(double delta, Input* input, Vector2& velocity) override {
		if (summon_cooldown > 0.0f) summon_cooldown -= (float)delta;
		if (teleport_cooldown > 0.0f) teleport_cooldown -= (float)delta;
		if (q_cooldown > 0.0f) q_cooldown -= (float)delta;
		if (f_cooldown > 0.0f) f_cooldown -= (float)delta;
		if (g_cooldown > 0.0f) g_cooldown -= (float)delta;
		
		if (action_timer > 0.0f) {
			action_timer -= (float)delta;
			if (action_timer <= 0.0f) { is_summoning = false; is_teleporting = false; }
		}

		if (core->key_q_just_pressed && core->has_mouse_teleport) {
			if (is_teleport_stance) is_teleport_stance = false; 
			else if (q_cooldown <= 0.0f) { is_teleport_stance = true; is_f_primed = false; is_g_primed = false; }
		}
		if (core->key_f_just_pressed && core->has_ice && f_cooldown <= 0.0f && !is_teleport_stance) {
			is_f_primed = true; is_g_primed = false;
		}
		if (core->key_g_just_pressed && core->has_fireball && g_cooldown <= 0.0f && !is_teleport_stance) {
			is_g_primed = true; is_f_primed = false;
		}

		if (core->mouse_left_just_pressed) {
			if (is_teleport_stance) {
				core->self->set_global_position(core->self->get_global_mouse_position()); 
				is_teleport_stance = false; q_cooldown = 10.0f;                
			} 
			else if (is_f_primed) {
				Ref<PackedScene> ice_scene = ResourceLoader::get_singleton()->load("res://assetLvl3/ice_freeze.tscn");
				if (ice_scene.is_valid()) {
					Node* proj = ice_scene->instantiate();
					if (proj) {
						Object::cast_to<Node2D>(proj)->set_meta("target_pos", core->self->get_global_mouse_position());
						core->self->get_tree()->get_current_scene()->add_child(proj);
						Object::cast_to<Node2D>(proj)->set_global_position(core->self->get_global_position());
					}
				}
				is_f_primed = false; f_cooldown = 30.0f;
				if (core->sprite) core->sprite->play("wizard_cast");
			} 
			else if (is_g_primed) {
				Ref<PackedScene> fire_scene = ResourceLoader::get_singleton()->load("res://assetLvl3/fireball.tscn");
				if (fire_scene.is_valid()) {
					Node* proj = fire_scene->instantiate();
					if (proj) {
						Object::cast_to<Node2D>(proj)->set_meta("target_pos", core->self->get_global_mouse_position());
						core->self->get_tree()->get_current_scene()->add_child(proj);
						Object::cast_to<Node2D>(proj)->set_global_position(core->self->get_global_position());
					}
				}
				is_g_primed = false; g_cooldown = 60.0f;
				if (core->sprite) core->sprite->play("wizard_cast");
			}
			else if (core->has_summon && summon_cooldown <= 0.0f && !is_action_locked() && core->self->is_on_floor()) {
				is_summoning = true; action_timer = 1.6f; summon_cooldown = 3.0f; 
				core->play_sfx(core->sfx_wizard_summon);
				if (core->sprite) { core->sprite->stop(); last_attack_frame = -1; }
			}
		}

		if (core->key_e_just_pressed && core->has_teleport && teleport_cooldown <= 0.0f && !is_action_locked() && core->self->is_on_floor()) {
			is_teleporting = true; action_timer = 1.4f; teleport_cooldown = 2.0f; 
			core->play_sfx(core->sfx_wizard_teleport);
			if (core->sprite) { core->sprite->stop(); last_attack_frame = -1; }
		}

		float direction = input->get_axis("ui_left", "ui_right");
		if (is_action_locked()) {
			velocity.x = 0; 
		} else {
			if (direction != 0) {
				velocity.x = direction * core->speed;
				if (core->sprite) core->sprite->set_flip_h(direction < 0);
			} else {
				velocity.x = UtilityFunctions::move_toward(velocity.x, 0, core->speed);
			}
		}

		if (core->sprite) {
			if (is_summoning) {
				core->sprite->play("wizard_summon");
				int current_frame = core->sprite->get_frame();
				if (current_frame == 7 && last_attack_frame < 7) {
					Ref<PackedScene> block_scene = ResourceLoader::get_singleton()->load("res://scene/wizard_block.tscn");
					if (block_scene.is_valid()) {
						Node* block_inst = block_scene->instantiate();
						if (block_inst) {
							bool facing_right = !core->sprite->is_flipped_h();
							core->self->get_tree()->get_current_scene()->add_child(block_inst);
							Object::cast_to<Node2D>(block_inst)->set_global_position(core->self->get_global_position() + (facing_right ? Vector2(50, -20) : Vector2(-50, -20)));
							core->self->get_tree()->create_timer(12.0)->connect("timeout", Callable((Object*)block_inst, StringName("queue_free")));
						}
					}
				}
				last_attack_frame = current_frame;
			} else if (is_teleporting) {
				core->sprite->play("wizard_teleport");
				int current_frame = core->sprite->get_frame();
				if (current_frame == 3 && last_attack_frame < 3) {
					bool facing_right = !core->sprite->is_flipped_h();
					Vector2 new_pos = core->self->get_global_position();
					new_pos.x += facing_right ? (7.0f * 16.0f) : -(7.0f * 16.0f);
					core->self->set_global_position(new_pos);
				}
				last_attack_frame = current_frame;
			} else if (is_teleport_stance) {
				core->sprite->play("wizard_idle");
			} else if (core->sprite->get_animation() != StringName("wizard_cast")) {
				last_attack_frame = -1;
				if (!core->self->is_on_floor()) core->sprite->play("wizard_jump");
				else if (velocity.x != 0) core->sprite->play("wizard_walk");
				else core->sprite->play("wizard_idle"); 
			}
		}
		core->stop_sfx(core->sfx_walk); 
	}
};

void PlayerController::switch_hero(int type) {
	if (type == 0) active_hero = std::make_unique<Knight>(this);
	else if (type == 1) active_hero = std::make_unique<Archer>(this);
	else if (type == 2) active_hero = std::make_unique<Wizard>(this);
	else if (type == 3) active_hero = std::make_unique<Priest>(this);
}

void PlayerController::on_ready(CharacterBody2D* node) {
	self = node;
	self->add_to_group("player");
	for (int i = 0; i < self->get_child_count(); i++) {
		Node* child = self->get_child(i);
		if (Object::cast_to<AnimatedSprite2D>(child)) { sprite = (AnimatedSprite2D*)child; break; }
	}

	start_pos = self->get_global_position();
	player_health = 50;
	
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

	if (bg_music) { bg_music->set_volume_db(0.0f); bg_music->play(); }
	if (combat_music) { combat_music->set_volume_db(-60.0f); combat_music->play(); }

	if (admin_mode) {
		unlocked_knight = true; unlocked_archer = true; unlocked_priest = true; unlocked_wizard = true;
		has_sword = true; has_shield = true; has_bow = true; has_grapple = true;
		has_heal = true; has_buff = true; has_summon = true; has_teleport = true;
		has_mouse_teleport = true; has_ice = true; has_fireball = true;
	} else {
		Engine* engine = Engine::get_singleton();
		has_sword = engine->has_meta("save_has_sword") ? (bool)engine->get_meta("save_has_sword") : false;
		has_shield = engine->has_meta("save_has_shield") ? (bool)engine->get_meta("save_has_shield") : false;
		has_bow = engine->has_meta("save_has_bow") ? (bool)engine->get_meta("save_has_bow") : false;
		has_grapple = engine->has_meta("save_has_grapple") ? (bool)engine->get_meta("save_has_grapple") : false;
		
		String scene_name = self->get_tree()->get_current_scene()->get_name();
		
		if (scene_name == "level2" || scene_name == "Level2") {
			unlocked_archer = true; has_sword = true; has_shield = true;
			unlocked_priest = false; unlocked_wizard = false;
		} else if (scene_name == "level3" || scene_name == "Level3") {
			unlocked_archer = true; has_sword = true; has_shield = true; has_bow = true; has_grapple = true;
			unlocked_priest = false; unlocked_wizard = true;
			has_mouse_teleport = true; has_ice = true; has_fireball = true;
		} else if (scene_name == "level4" || scene_name == "Level4") {
			unlocked_archer = true; unlocked_priest = true; unlocked_wizard = true; 
			has_sword = true; has_shield = true; has_summon = true; has_teleport = true;
		} else {
			unlocked_archer = false; unlocked_priest = false; unlocked_wizard = false;
		}
	}
	switch_hero(0); 
}

void PlayerController::on_physics_process(double delta) {
	if (!self || is_dead) return;

	if (damage_flash_timer > 0.0f) {
		damage_flash_timer -= (float)delta;
		if (damage_flash_timer <= 0.0f && sprite) sprite->set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
	}

	bool enemies_near = false;
	TypedArray<Node> enemies = self->get_tree()->get_nodes_in_group("enemy");
	for (int i = 0; i < enemies.size(); i++) {
		Node2D* enemy = Object::cast_to<Node2D>(enemies[i]);
		if (enemy && self->get_global_position().distance_to(enemy->get_global_position()) < 150.0f) {
			enemies_near = true; break;
		}
	}

	if (enemies_near) combat_cooldown = 2.0f; 
	else if (combat_cooldown > 0.0f) combat_cooldown -= (float)delta;

	if (bg_music && combat_music) {
		bool play_combat = (combat_cooldown > 0.0f);
		float target_bg_vol = play_combat ? -15.0f : 0.0f;     
		float target_combat_vol = play_combat ? 0.0f : -60.0f; 
		float current_bg = bg_music->get_volume_db();
		float current_combat = combat_music->get_volume_db();
		
		bg_music->set_volume_db(current_bg + (target_bg_vol - current_bg) * 2.0f * (float)delta);
		combat_music->set_volume_db(current_combat + (target_combat_vol - current_combat) * 2.0f * (float)delta);
	}

	Input* input = Input::get_singleton();
	
	bool curr_mouse = input->is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT);
	mouse_left_just_pressed = curr_mouse && !prev_mouse; prev_mouse = curr_mouse;

	bool curr_q = input->is_key_pressed(Key::KEY_Q);
	key_q_just_pressed = curr_q && !prev_key_q; prev_key_q = curr_q;

	bool curr_e = input->is_key_pressed(Key::KEY_E);
	key_e_just_pressed = curr_e && !prev_key_e; prev_key_e = curr_e;

	bool curr_f = input->is_key_pressed(Key::KEY_F);
	key_f_just_pressed = curr_f && !prev_key_f; prev_key_f = curr_f;

	bool curr_g = input->is_key_pressed(Key::KEY_G);
	key_g_just_pressed = curr_g && !prev_key_g; prev_key_g = curr_g;

	if (buff_active_timer > 0.0f) buff_active_timer -= (float)delta;

	if (active_hero && !active_hero->is_action_locked()) {
		if (input->is_action_just_pressed("hero_1") && unlocked_knight) switch_hero(0);
		if (input->is_action_just_pressed("hero_2") && unlocked_archer) switch_hero(1);
		if (input->is_action_just_pressed("hero_3") && unlocked_wizard) switch_hero(2);
		if (input->is_action_just_pressed("hero_4") && unlocked_priest) switch_hero(3);
	}

	Vector2 velocity = self->get_velocity();
	if (!self->is_on_floor()) velocity.y += gravity * (float)delta;

	if (active_hero && !active_hero->is_action_locked() && input->is_action_just_pressed("ui_accept") && self->is_on_floor()) {
		velocity.y = jump_velocity;
	}

	if (active_hero) active_hero->update(delta, input, velocity);

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
					if (normal.x > 0.5f || normal.x < -0.5f) rb->apply_central_impulse(Vector2(-normal.x * 150.0f, 0));
				}
			}
		}
	}

	bool is_on_floor = self->is_on_floor();
	if (!was_on_floor && is_on_floor && sfx_landing) sfx_landing->play();
	was_on_floor = is_on_floor;
}

static std::unordered_map<uint64_t, std::shared_ptr<PlayerController>> player_instances;
static PlayerController* fallback_player = nullptr; 

std::shared_ptr<PlayerController> get_player(Caller* instance) {
	if (!instance) return nullptr;
	CharacterBody2D* node = GetSelf<CharacterBody2D>(instance);
	if (!node) return nullptr;
	uint64_t id = node->get_instance_id();
	if (player_instances.find(id) == player_instances.end()) {
		player_instances[id] = std::make_shared<PlayerController>();
		fallback_player = player_instances[id].get(); 
	}
	return player_instances[id];
}

JENOVA_SCRIPT_BEGIN
void unlock_sword(Caller* instance) { if (auto p = get_player(instance)) p->has_sword = true; Engine::get_singleton()->set_meta("save_has_sword", true); }
void unlock_shield(Caller* instance) { if (auto p = get_player(instance)) p->has_shield = true; Engine::get_singleton()->set_meta("save_has_shield", true); }
void unlock_bow(Caller* instance) { if (auto p = get_player(instance)) p->has_bow = true; Engine::get_singleton()->set_meta("save_has_bow", true); }
void unlock_grapple(Caller* instance) { if (auto p = get_player(instance)) p->has_grapple = true; Engine::get_singleton()->set_meta("save_has_grapple", true); }
void unlock_heal(Caller* instance) { if (auto p = get_player(instance)) p->has_heal = true; Engine::get_singleton()->set_meta("save_has_heal", true); }
void unlock_buff(Caller* instance) { if (auto p = get_player(instance)) p->has_buff = true; Engine::get_singleton()->set_meta("save_has_buff", true); }
void unlock_summon(Caller* instance) { if (auto p = get_player(instance)) p->has_summon = true; Engine::get_singleton()->set_meta("save_has_summon", true); }
void unlock_teleport(Caller* instance) { if (auto p = get_player(instance)) p->has_teleport = true; Engine::get_singleton()->set_meta("save_has_teleport", true); }
int increase_inventory(Caller* instance) { if (auto p = get_player(instance)) { p->inventory_count += 1; return p->inventory_count; } return 0; }
int get_inventory_count(Caller* instance) { if (auto p = get_player(instance)) return p->inventory_count; return 0; }
void actually_teleport(Caller* instance) { if (auto p = get_player(instance)) p->actually_teleport(); }
void respawn() { if (fallback_player) fallback_player->respawn(); }
bool take_damage(int amount) { return fallback_player ? fallback_player->take_damage(amount) : false; }

void OnAwake(Caller* instance) { 
	get_player(instance);
}
void OnReady(Caller* instance) { 
	if (auto p = get_player(instance)) p->on_ready(GetSelf<CharacterBody2D>(instance)); 
}
void OnPhysicsProcess(Caller* instance, double delta) { 
	if (auto p = get_player(instance)) p->on_physics_process(delta); 
}
JENOVA_SCRIPT_END
