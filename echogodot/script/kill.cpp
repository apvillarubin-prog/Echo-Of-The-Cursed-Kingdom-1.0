#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// --- [NEW] Cooldown Logic ---
	// Check if the spikes are currently "asleep" after hitting the player
	float cooldown = self->has_meta("kill_cooldown") ? (float)self->get_meta("kill_cooldown") : 0.0f;
	if (cooldown > 0.0f) {
		cooldown -= (float)delta;
		self->set_meta("kill_cooldown", cooldown);
		return; // Stop processing entirely until the cooldown is finished!
	}
	// -----------------------------

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body) {
			
			if (body->get_name() == String("knight") || body->is_in_group("player")) {
				
				UtilityFunctions::print("[DEBUG-KILLBLOCK] Player hit spikes! Going to sleep for 2 seconds.");
				body->call("take_damage", 9999);
				
				// --- [NEW] Trigger the cooldown ---
				// Put the spikes to sleep for 2 seconds so the UI can breathe
				self->set_meta("kill_cooldown", 2.0f);
				
				break; 
			}
		}
	}
}

JENOVA_SCRIPT_END
