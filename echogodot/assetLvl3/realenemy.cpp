#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/sprite_frames.hpp> 
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/progress_bar.hpp>
#include <Godot/classes/ray_cast2d.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <Godot/classes/collision_shape2d.hpp>
#include <Godot/classes/tween.hpp>
#include <Godot/classes/property_tweener.hpp>
#include <Godot/classes/callback_tweener.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/callable.hpp>
#include <unordered_map>
#include <memory>

using namespace godot;
using namespace jenova::sdk;

class EnemyController {
public:
	CharacterBody2D* self = nullptr;
	AnimatedSprite2D* anim = nullptr;
	ProgressBar* hp_bar = nullptr;
	RayCast2D* vision_ray = nullptr;
	AudioStreamPlayer2D* hit_audio = nullptr;

	// --- Constants from Script 1 ---
	float AGGRO_RANGE = 200.0f;
	float CHASE_SPEED = 40.0f;
	float PATROL_SPEED = 25.0f;
	float GRAVITY = 1000.0f;
	float KNOCKBACK_FORCE = 180.0f;
	float KNOCKBACK_LIFT = -120.0f;
	float MAX_AGGRO_Y = 50.0f;
	float ATTACK_RANGE = 30.0f;
	int MAX_HEALTH = 40;

	// --- State Trackers ---
	int current_health = 40;
	bool is_dying = false;
	bool is_frozen = false;
	float attack_cooldown = 0.0f;
	int last_hit_frame = -1;
	int patrol_direction = 1;
	float knockback_timer = 0.0f;

	void die() {
		is_dying = true;
		self->set_physics_process(false);
		self->set_velocity(Vector2(0, 0));

		CollisionShape2D* shape = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
		if (shape) shape->set_deferred("disabled", true);

		if (anim) anim->set_modulate(Color(5.0f, 0.4f, 0.4f, 1.0f));

		// "Juicy" Death Tween
		Ref<Tween> tween = self->create_tween();
		if (tween.is_valid()) {
			tween->set_parallel(true);
			float spin_angle = UtilityFunctions::randf_range(-6.0f, 6.0f);
			
			tween->tween_property(self, "rotation", spin_angle, 0.35f);
			tween->tween_property(self, "scale", Vector2(0, 0), 0.35f)->set_trans(Tween::TRANS_BACK)->set_ease(Tween::EASE_IN);
			tween->tween_property(anim, "modulate:a", 0.0f, 0.35f);
			
			tween->chain()->tween_callback(Callable(self, StringName("queue_free")));
		} else {
			self->queue_free(); // Fallback if tween fails
		}
	}

	void freeze_enemy() {
		is_frozen = true;
		self->set_velocity(Vector2(0, 0));
		self->set_physics_process(false);
		self->set_process(false);

		if (anim) {
			anim->stop();
			anim->set_modulate(Color(0.5f, 0.7f, 1.0f, 1.0f)); 
		}
	}

	void on_ready(CharacterBody2D* node) {
		self = node;
		if (!self->is_in_group("enemy")) self->add_to_group("enemy");
		self->set_collision_mask_value(2, false);

		anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
		hp_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("HealthBar"));
		vision_ray = Object::cast_to<RayCast2D>(self->get_node_or_null("RayCast2D"));
		hit_audio = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("HitAudio"));

		current_health = MAX_HEALTH;
		if (hp_bar) {
			hp_bar->set("max_value", (double)MAX_HEALTH);
			hp_bar->set("value", (double)current_health);
		}
	}

	void on_physics_process(double delta) {
		if (!self || is_dying || is_frozen) return;

		Vector2 velocity = self->get_velocity();

		// --- Knockback Stun State Override ---
		if (knockback_timer > 0.0f) {
			knockback_timer -= (float)delta;
			if (!self->is_on_floor()) velocity.y += GRAVITY * (float)delta;
			self->set_velocity(velocity);
			self->move_and_slide();
			return; // Prevents AI from overriding the knockback trajectory
		}

		// --- Damage Registration ---
		if (self->has_meta("pending_damage")) {
			int incoming_dmg = (int)self->get_meta("pending_damage");
			self->remove_meta("pending_damage"); 
			current_health -= incoming_dmg;

			if (hp_bar) {
				hp_bar->set("max_value", (double)MAX_HEALTH); 
				hp_bar->set("value", (double)current_health);
			}

			if (current_health <= 0) {
				die();
				return;
			} else {
				if (hit_audio) hit_audio->play();

				TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
				if (players.size() > 0) {
					Node2D* player = Object::cast_to<Node2D>(players[0]);
					int push_dir = (self->get_global_position().x > player->get_global_position().x) ? 1 : -1;
					
					velocity.x = push_dir * KNOCKBACK_FORCE;
					velocity.y = KNOCKBACK_LIFT; 
					knockback_timer = 0.15f; 
					
					if (anim) anim->play("enemy_idle"); 
				}
			}
		}

		if (attack_cooldown > 0.0f) attack_cooldown -= (float)delta;
		if (!self->is_on_floor()) velocity.y += GRAVITY * (float)delta;

		// --- Target Tracking & Line of Sight ---
		Node2D* target = nullptr;
		TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
		if (players.size() > 0) target = Object::cast_to<Node2D>(players[0]);

		bool can_see_player = false;
		float dist_x = 0.0f;
		float dist_y = 0.0f;

		if (target) {
			Vector2 p_pos = target->get_global_position();
			Vector2 e_pos = self->get_global_position();
			dist_x = std::abs(p_pos.x - e_pos.x);
			dist_y = std::abs(p_pos.y - e_pos.y);

			if (dist_x <= AGGRO_RANGE && dist_y <= MAX_AGGRO_Y) {
				if (vision_ray) {
					vision_ray->set_target_position(vision_ray->to_local(p_pos));
					vision_ray->force_raycast_update();
					if (vision_ray->is_colliding()) {
						Node* collider = Object::cast_to<Node>(vision_ray->get_collider());
						if (collider && collider->is_in_group("player")) can_see_player = true;
					} else {
						can_see_player = true;
					}
				} else {
					can_see_player = true;
				}
			}
		}

		// --- AGGRO STATE ---
		if (can_see_player && target) {
			Vector2 p_pos = target->get_global_position();
			int dir = (p_pos.x > self->get_global_position().x) ? 1 : -1;

			bool is_attacking = (anim && anim->get_animation() == StringName("enemy_attack"));

			if (attack_cooldown <= 0.0f || !is_attacking) {
				if (anim) anim->set_flip_h(dir < 0);
			}

			if (dist_x > ATTACK_RANGE && attack_cooldown <= 0.0f) {
				velocity.x = dir * CHASE_SPEED;
				if (anim) anim->play("enemy_run");
				last_hit_frame = -1;
			} else {
				velocity.x = 0; 
				if (attack_cooldown <= 0.0f) {
					if (anim) anim->play("enemy_attack");
				}

				if (anim && anim->get_animation() == StringName("enemy_attack")) {
					int current_frame = anim->get_frame();
					if (current_frame == 2 && last_hit_frame != 2) {
						if (dist_x <= ATTACK_RANGE + 10.0f && dist_y <= MAX_AGGRO_Y) {
							target->call("take_damage", 1);
						}
						last_hit_frame = 2;
						attack_cooldown = 1.5f; 
					} else if (current_frame != 2) {
						last_hit_frame = -1;
					}
				} else if (attack_cooldown > 0.0f) {
					if (anim) anim->play("enemy_idle");
				}
			}
		} 
		// --- PATROL STATE ---
		else {
			velocity.x = patrol_direction * PATROL_SPEED;
			if (anim) {
				anim->play("enemy_run");
				anim->set_flip_h(patrol_direction < 0);
			}
			last_hit_frame = -1;
		}

		self->set_velocity(velocity);
		self->move_and_slide();

		// Wall Turnaround
		if (self->is_on_wall() && !can_see_player) {
			patrol_direction *= -1;
		}
	}
};



static std::unordered_map<uint64_t, std::shared_ptr<EnemyController>> enemy_instances;

std::shared_ptr<EnemyController> get_enemy(Caller* instance) {
	if (!instance) return nullptr;
	CharacterBody2D* node = GetSelf<CharacterBody2D>(instance);
	if (!node) return nullptr;

	uint64_t id = node->get_instance_id();
	if (enemy_instances.find(id) == enemy_instances.end()) {
		enemy_instances[id] = std::make_shared<EnemyController>();
	}
	return enemy_instances[id];
}

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	if (auto e = get_enemy(instance)) e->on_ready(GetSelf<CharacterBody2D>(instance));
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (auto e = get_enemy(instance)) e->on_physics_process(delta);
}

// Allows other scripts to freeze this enemy (e.g., Wizard's Ice spell)
void freeze_enemy(Caller* instance) {
	if (auto e = get_enemy(instance)) e->freeze_enemy();
}

JENOVA_SCRIPT_END
