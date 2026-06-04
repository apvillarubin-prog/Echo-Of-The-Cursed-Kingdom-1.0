#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Area2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;

Vector2 target_position;
float fly_speed = 500.0f;
bool initialized = false;
bool is_exploding = false;

float max_flight_lifetime = 3.0f; 
float explosion_cleanup_timer = 0.6f; // Manual countdown guarantees self-destruction

void OnReady(Caller* instance) {
	self = GetSelf<Area2D>(instance);
	if (self) {
		sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
		
		if (self->has_meta("target_pos")) {
			target_position = self->get_meta("target_pos");
			initialized = true;
			Vector2 dir = (target_position - self->get_global_position()).normalized();
			self->set_rotation(dir.angle());
		}
		if (sprite) sprite->play("default");
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || !initialized) return;

	// Phase 2: Handle Explosion Sequence
	if (is_exploding) {
		explosion_cleanup_timer -= (float)delta;
		if (explosion_cleanup_timer <= 0.0f) {
			self->queue_free(); // Cleanly erase from memory
		}
		return;
	}

	// Safety out-of-bounds boundary fuse
	max_flight_lifetime -= (float)delta;
	if (max_flight_lifetime <= 0.0f) {
		self->queue_free();
		return;
	}

	Vector2 current_pos = self->get_global_position();
	
	// Check if it reached the target destination coordinates
	if (current_pos.distance_squared_to(target_position) < 225.0f) {
		is_exploding = true;
		self->set_rotation(0.0f);
		if (sprite) sprite->play("default");
	} else {
		Vector2 direction = (target_position - current_pos).normalized();
		self->set_global_position(current_pos + (direction * fly_speed * (float)delta));
	}
}

// NEW: Instantly explodes if it intersects an enemy physics collision shape
void _on_body_entered(Node* body) {
	if (!self || is_exploding) return;

	if (body->is_in_group("enemy")) {
		body->call("take_damage", 15); // Deal fireball damage
		is_exploding = true;
		self->set_rotation(0.0f); // Keep visual blast vertical
		if (sprite) sprite->play("default");
	}
}

JENOVA_SCRIPT_END
