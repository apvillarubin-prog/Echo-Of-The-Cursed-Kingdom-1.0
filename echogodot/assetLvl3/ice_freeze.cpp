#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/packed_scene.hpp>
#include <Godot/classes/resource_loader.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Area2D* self = nullptr;
Vector2 target_position;
float speed = 400.0f;
bool initialized = false;
float safety_lifetime = 4.0f; 

void OnReady(Caller* instance) {
	self = GetSelf<Area2D>(instance);
	if (self && self->has_meta("target_pos")) {
		target_position = self->get_meta("target_pos");
		initialized = true;
		
		Vector2 current_pos = self->get_global_position();
		Vector2 direction = (target_position - current_pos).normalized();
		self->set_rotation(direction.angle());
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || !initialized) return;

	safety_lifetime -= (float)delta;
	if (safety_lifetime <= 0.0f) {
		self->queue_free();
		return;
	}

	Vector2 current_pos = self->get_global_position();
	
	// Check if it reached the destination
	if (current_pos.distance_squared_to(target_position) < 100.0f) {
		self->queue_free(); 
	} else {
		Vector2 direction = (target_position - current_pos).normalized();
		self->set_global_position(current_pos + (direction * speed * (float)delta));
	}
}

void _on_body_entered(Node* body) {
	if (!self || !body) return;
	
	// Check for enemy group
	if (body->is_in_group("enemy")) {
		// Calls the function in the enemy script
		body->call("freeze_enemy"); 
		self->queue_free(); 
	}
}

// THIS MUST BE AT THE VERY END
JENOVA_SCRIPT_END
