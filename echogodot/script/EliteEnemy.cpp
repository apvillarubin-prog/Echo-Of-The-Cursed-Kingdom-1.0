#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <map>

using namespace godot;
using namespace jenova::sdk;

struct GoblinData {
	int health = 10;
	int last_hit_frame = -1;
	bool is_dying = false;
	Node2D* target = nullptr;
	float attack_cooldown = 0.0f;

	const float AGGRO_RANGE = 200.0f;
	const float CHASE_SPEED = 30.0f;
	const float ATTACK_RANGE = 40.0f;
	const float ATTACK_COOLDOWN_DURATION = 3.0f;
};

static std::map<uint64_t, GoblinData*> registry;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) {
		UtilityFunctions::print("DEBUG [MiniBoss] OnReady — self is NULL, wrong node type!");
		return;
	}

	uint64_t id = self->get_instance_id();
	UtilityFunctions::print("DEBUG [MiniBoss] OnReady fired. Instance ID: ", (int64_t)id);

	registry[id] = new GoblinData();

	if (!self->is_in_group("enemy")) self->add_to_group("enemy");
	self->set_collision_mask_value(2, false);
	UtilityFunctions::print("DEBUG [MiniBoss] Registered and added to group 'enemy'.");
}

void take_hit(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) {
		UtilityFunctions::print("DEBUG [MiniBoss] take_hit — self is NULL!");
		return;
	}

	uint64_t id = self->get_instance_id();
	UtilityFunctions::print("DEBUG [MiniBoss] take_hit entered. Instance ID: ", (int64_t)id);

	if (registry.count(id)) {
		GoblinData* data = registry[id];
		UtilityFunctions::print("DEBUG [MiniBoss] Found in registry. is_dying=", data->is_dying, " HP=", data->health);

		if (data->is_dying) {
			UtilityFunctions::print("DEBUG [MiniBoss] Already dying, ignoring hit.");
			return;
		}

		data->health -= 1;
		UtilityFunctions::print("MiniBoss Hit! HP: ", data->health);

		if (data->health <= 0) {
			UtilityFunctions::print("DEBUG [MiniBoss] HP reached 0, calling queue_free.");
			data->is_dying = true;
			self->queue_free();
		}
	} else {
		UtilityFunctions::print("DEBUG [MiniBoss] ERROR — ID not in registry! ID: ", (int64_t)id);
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	uint64_t id = self->get_instance_id();

	if (registry.find(id) == registry.end()) {
		UtilityFunctions::print("DEBUG [MiniBoss] OnPhysicsProcess — ID not in registry!");
		return;
	}

	GoblinData* data = registry[id];
	if (data->is_dying) return;

	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (!anim) {
		UtilityFunctions::print("DEBUG [MiniBoss] AnimatedSprite2D not found!");
		return;
	}

	Vector2 velocity = self->get_velocity();

	// tick attack cooldown
	if (data->attack_cooldown > 0.0f) {
		data->attack_cooldown -= (float)delta;
		if (data->attack_cooldown < 0.0f) data->attack_cooldown = 0.0f;
	}

	if (data->target && !ObjectDB::get_instance(data->target->get_instance_id())) {
		data->target = nullptr;
	}

	if (!data->target) {
		TypedArray<Node> players = self->get_tree()->get_nodes_in_group("player");
		if (players.size() > 0) {
			data->target = Object::cast_to<Node2D>(players[0]);
		}
	}

	if (data->target) {
		Vector2 p_pos = data->target->get_global_position();
		Vector2 e_pos = self->get_global_position();
		float dist = e_pos.distance_to(p_pos);

		if (dist <= data->AGGRO_RANGE) {
			int dir = (p_pos.x > e_pos.x) ? 1 : -1;
			anim->set_flip_h(dir < 0);

			if (dist > data->ATTACK_RANGE) {
				// chase player
				velocity.x = dir * data->CHASE_SPEED;
				anim->play("enemy_run");
				data->last_hit_frame = -1;
			} else {
				velocity.x = 0;

				if (data->attack_cooldown <= 0.0f) {
					// cooldown finished, can attack
					anim->play("enemy_attack");

					int current_frame = anim->get_frame();
					if (current_frame == 9 && data->last_hit_frame != 9) {
						float hit_dist = e_pos.distance_to(data->target->get_global_position());
						if (hit_dist <= 50.0f) {
							UtilityFunctions::print("DEBUG [MiniBoss] Attack hit! Dealing 10 damage.");
							data->target->call("take_damage", 10);
							// start cooldown after landing a hit
							data->attack_cooldown = data->ATTACK_COOLDOWN_DURATION;
							UtilityFunctions::print("DEBUG [MiniBoss] Attack cooldown started: 3s");
						}
						data->last_hit_frame = 2;
					} else if (current_frame != 2) {
						data->last_hit_frame = -1;
					}
				} else {
					// on cooldown, just idle in place
					anim->play("enemy_idle");
					UtilityFunctions::print("DEBUG [MiniBoss] Attack on cooldown: ", data->attack_cooldown, "s remaining");
					data->last_hit_frame = -1;
				}
			}
		} else {
			velocity.x = 0;
			anim->play("enemy_idle");
			data->last_hit_frame = -1;
		}
	}

	if (!self->is_on_floor()) velocity.y += 1000.0f * (float)delta;
	self->set_velocity(velocity);
	self->move_and_slide();
}

void OnDestroy(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	uint64_t id = self->get_instance_id();

	if (registry.count(id)) {
		delete registry[id];
		registry.erase(id);
		UtilityFunctions::print("DEBUG [MiniBoss] Removed from registry.");
	}
}

JENOVA_SCRIPT_END
