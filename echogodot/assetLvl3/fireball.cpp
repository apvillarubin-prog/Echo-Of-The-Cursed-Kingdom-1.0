#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Area2D* self = nullptr;
AnimatedSprite2D* sprite = nullptr;
AudioStreamPlayer2D* launch_sound = nullptr;
AudioStreamPlayer2D* explode_sound = nullptr;

Vector2 target_position;
float fly_speed = 500.0f;
bool initialized = false;
bool is_exploding = false;
float max_flight_lifetime = 3.0f; 
float explosion_cleanup_timer = 0.6f; 

// --- FIX: Moved this function ABOVE the others so the compiler knows it exists ---
void trigger_explosion() {
	if (is_exploding) return;
	
	is_exploding = true;
	if (self) self->set_rotation(0.0f);
	
	if (sprite) sprite->play("default"); 
	if (explode_sound) explode_sound->play();
}

void OnReady(Caller* instance) {
	self = GetSelf<Area2D>(instance);
	if (self) {
		sprite = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
		launch_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("LaunchSound"));
		explode_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("ExplodeSound"));

		is_exploding = false;
		initialized = false;
		max_flight_lifetime = 3.0f;
		explosion_cleanup_timer = 0.6f;

		if (self->has_meta("target_pos")) {
			target_position = self->get_meta("target_pos");
			initialized = true;
			Vector2 dir = (target_position - self->get_global_position()).normalized();
			self->set_rotation(dir.angle());
			if (launch_sound) launch_sound->play();
		}
		if (sprite) sprite->play("default");
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || !initialized) return;

	if (is_exploding) {
		explosion_cleanup_timer -= (float)delta;
		if (explosion_cleanup_timer <= 0.0f) {
			self->queue_free();
		}
		return;
	}

	max_flight_lifetime -= (float)delta;
	if (max_flight_lifetime <= 0.0f) {
		self->queue_free();
		return;
	}

	Vector2 current_pos = self->get_global_position();
	if (current_pos.distance_squared_to(target_position) < 225.0f) {
		trigger_explosion();
	} else {
		Vector2 direction = (target_position - current_pos).normalized();
		self->set_global_position(current_pos + (direction * fly_speed * (float)delta));
	}
}

void _on_body_entered(Node* body) {
	if (!self || is_exploding) return;

	if (body->is_in_group("enemy")) {
		body->call("take_damage", 15);  
		trigger_explosion();
	}
}

JENOVA_SCRIPT_END
