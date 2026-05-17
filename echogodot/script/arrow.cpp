/* Jenova C++ Node Base Script (Arrow) */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

float speed = 350.0f; 

void OnPhysicsProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	Vector2 current_scale = self->get_scale();
	Vector2 move_direction = (current_scale.x < 0.0f) ? Vector2(-1, 0) : Vector2(1, 0);

	Vector2 current_pos = self->get_global_position();
	self->set_global_position(current_pos + (move_direction * speed * (float)delta));

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (!body) continue;

		if (body->is_in_group("player")) continue;

		if (body->is_in_group("enemy")) {
			// FIX: Set meta values to handle archer damage instantly
			body->set_meta("pending_damage", 5);
			self->queue_free(); 
			return;
		}

		if (body->is_in_group("lever")) {
			body->call("activate");
			self->queue_free(); 
			return;
		}

		if (body->get_class() == "TileMap" || body->get_class() == "StaticBody2D") {
			self->queue_free(); 
			return;
		}
	}
}

JENOVA_SCRIPT_END
