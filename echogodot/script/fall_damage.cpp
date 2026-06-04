#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// --- [NEW] Add the spikes to a group on startup ---
void OnReady(Caller* instance) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (self) {
		self->add_to_group("spikes");
	}
}
// --------------------------------------------------

void OnProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Check if these spikes have already dealt damage
	bool already_triggered = self->has_meta("already_triggered") ? (bool)self->get_meta("already_triggered") : false;
	if (already_triggered) {
		return; 
	}

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body) {
			
			if (body->get_name() == String("knight") || body->is_in_group("player")) {
				
				int current_health = body->has_meta("health") ? (int)body->get_meta("health") : 50; 
				int damage_to_deal = (int)(current_health * 0.90f);
				
				if (damage_to_deal <= 0 && current_health > 0) {
					damage_to_deal = 1;
				}

				UtilityFunctions::print("[DEBUG-DAMAGEBLOCK] Player hit spikes! Dealing 90% damage: ", damage_to_deal);
				
				body->call("take_damage", damage_to_deal);
				
				// Mark as triggered
				self->set_meta("already_triggered", true);
				
				break; 
			}
		}
	}
}

JENOVA_SCRIPT_END
