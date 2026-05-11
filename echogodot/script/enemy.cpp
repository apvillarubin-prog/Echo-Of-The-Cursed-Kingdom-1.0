#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <map>

using namespace godot;
using namespace jenova::sdk;

struct GoblinData {
	int health = 3;
	int last_hit_frame = -1;
	bool is_dying = false;
	Node2D* target = nullptr;

	const float AGGRO_RANGE = 200.0f;
	const float CHASE_SPEED = 40.0f;
	const float ATTACK_RANGE = 15.0f;
};

static std::map<uint64_t, GoblinData*> registry;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) {
		UtilityFunctions::print("DEBUG [Goblin] OnReady — self is NULL, wrong node type!");
		return;
	}

	uint64_t id = self->get_instance_id();
	UtilityFunctions::print("DEBUG [Goblin] OnReady fired. Instance ID: ", (int64_t)id);

	registry[id] = new GoblinData();

	if (!self->is_in_group("enemy")) self->add_to_group("enemy");
	self->set_collision_mask_value(2, false);
	UtilityFunctions::print("DEBUG [Goblin] Registered and added to group 'enemy'.");
}

void take_hit(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) {
		UtilityFunctions::print("DEBUG [Goblin] take_hit — self is NULL!");
		return;
	}

	uint64_t id = self->get_instance_id();
	UtilityFunctions::print("DEBUG [Goblin] take_hit entered. Instance ID: ", (int64_t)id);

	if (registry.count(id)) {
		GoblinData* data = registry[id];
		UtilityFunctions::print("DEBUG [Goblin] Found in registry. is_dying=", data->is_dying, " HP=", data->health);

		if (data->is_dying) {
			UtilityFunctions::print("DEBUG [Goblin] Already dying, ignoring hit.");
			return;
		}

		data->health -= 1;
		UtilityFunctions::print("Goblin Hit! HP: ", data->health);

		if (data->health <= 0) {
			UtilityFunctions::print("DEBUG [Goblin] HP reached 0, calling queue_free.");
			data->is_dying = true;
			self->queue_free();
		}
	} else {
		UtilityFunctions::print("DEBUG [Goblin] ERROR — ID not in registry! ID: ", (int64_t)id);
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	uint64_t id = self->get_instance_id();

	if (registry.find(id) == registry.end()) {
		UtilityFunctions::print("DEBUG [Goblin] OnPhysicsProcess — ID not in registry, OnReady never fired!");
		return;
	}

	GoblinData* data = registry[id];
	if (data->is_dying) return;

	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (!anim) {
		UtilityFunctions::print("DEBUG [Goblin] AnimatedSprite2D not found!");
		return;
	}

	Vector2 velocity = self->get_velocity();

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
				velocity.x = dir * data->CHASE_SPEED;
				anim->play("enemy_run");
				data->last_hit_frame = -1;
			} else {
				velocity.x = 0;
				anim->play("enemy_attack");

				int current_frame = anim->get_frame();
				if (current_frame == 2 && data->last_hit_frame != 2) {
					float hit_dist = e_pos.distance_to(data->target->get_global_position());
					if (hit_dist <= 30.0f) {
						data->target->call("take_damage", 1);
					}
					data->last_hit_frame = 2;
				} else if (current_frame != 2) {
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
		UtilityFunctions::print("DEBUG [Goblin] Removed from registry.");
	}
}

JENOVA_SCRIPT_END
