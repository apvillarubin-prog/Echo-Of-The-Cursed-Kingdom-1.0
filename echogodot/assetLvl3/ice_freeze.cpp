#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/packed_scene.hpp>
#include <Godot/classes/resource_loader.hpp>
#include <Godot/classes/audio_stream_player2d.hpp> // <-- Added for audio!
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Area2D* self = nullptr;
AudioStreamPlayer2D* freeze_sound = nullptr; // <-- The sound node variable

Vector2 target_position;
float speed = 400.0f;
bool initialized = false;
bool is_shattering = false; // <-- Tracks if the sound is playing
float safety_lifetime = 4.0f; 
float shatter_cleanup_timer = 0.6f; // <-- Timer to let the sound finish

// --- Triggers the sound and hides the ice visually ---
void trigger_shatter() {
	if (is_shattering) return;
	
	is_shattering = true;
	if (self) self->set_visible(false); // Make the ice disappear visually
	if (freeze_sound) freeze_sound->play(); // Play the freeze.mp3
}

void OnReady(Caller* instance) {
	self = GetSelf<Area2D>(instance);
	if (self) {
		// Look for a node named "FreezeSound"
		freeze_sound = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("FreezeSound"));
		
		is_shattering = false;
		initialized = false;
		safety_lifetime = 4.0f;
		shatter_cleanup_timer = 0.6f;

		if (self->has_meta("target_pos")) {
			target_position = self->get_meta("target_pos");
			initialized = true;
			
			Vector2 current_pos = self->get_global_position();
			Vector2 direction = (target_position - current_pos).normalized();
			self->set_rotation(direction.angle());
		}
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!self || !initialized) return;

	// If it hit something, wait for the sound to finish, then delete!
	if (is_shattering) {
		shatter_cleanup_timer -= (float)delta;
		if (shatter_cleanup_timer <= 0.0f) {
			self->queue_free();
		}
		return;
	}

	safety_lifetime -= (float)delta;
	if (safety_lifetime <= 0.0f) {
		self->queue_free();
		return;
	}

	Vector2 current_pos = self->get_global_position();
	
	// Check if it reached the destination
	if (current_pos.distance_squared_to(target_position) < 100.0f) {
		trigger_shatter(); // Play sound instead of deleting instantly
	} else {
		Vector2 direction = (target_position - current_pos).normalized();
		self->set_global_position(current_pos + (direction * speed * (float)delta));
	}
}

void _on_body_entered(Node* body) {
	if (!self || !body || is_shattering) return;
	
	// Check for enemy group
	if (body->is_in_group("enemy")) {
		// Calls the function in the enemy script
		body->call("freeze_enemy"); 
		trigger_shatter(); // Play sound instead of deleting instantly
	}
}

// THIS MUST BE AT THE VERY END
JENOVA_SCRIPT_END
