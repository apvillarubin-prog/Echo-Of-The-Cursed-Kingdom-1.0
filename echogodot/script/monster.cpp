/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/character_body2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <map>

using namespace godot;
using namespace jenova::sdk;

struct MonsterData {
	AnimatedSprite2D* sprite = nullptr;
	int direction = 1;
};

static std::map<uint64_t, MonsterData*> registry;

float speed = 50.0f;
float gravity = 980.0f;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) {
		UtilityFunctions::print("DEBUG [Monster] OnReady — self is NULL!");
		return;
	}

	uint64_t id = self->get_instance_id();
	MonsterData* data = new MonsterData();
	data->sprite = self->get_node<AnimatedSprite2D>("AnimatedSprite2D");
	data->direction = 1;
	registry[id] = data;

	UtilityFunctions::print("DEBUG [Monster] OnReady fired. Instance ID: ", (int64_t)id);
}

void OnProcess(Caller* instance, double delta) {
	CharacterBody2D* self = GetSelf<CharacterBody2D>(instance);
	if (!self) return;

	uint64_t id = self->get_instance_id();

	if (registry.find(id) == registry.end()) {
		UtilityFunctions::print("DEBUG [Monster] Not in registry, OnReady never fired!");
		return;
	}

	MonsterData* data = registry[id];

	Vector2 velocity = self->get_velocity();

	if (!self->is_on_floor()) velocity.y += gravity * (float)delta;

	if (self->is_on_wall()) {
		data->direction *= -1;
		if (data->sprite) data->sprite->set_flip_h(data->direction == -1);
	}

	velocity.x = data->direction * speed;
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
		UtilityFunctions::print("DEBUG [Monster] Removed from registry.");
	}
}

JENOVA_SCRIPT_END
