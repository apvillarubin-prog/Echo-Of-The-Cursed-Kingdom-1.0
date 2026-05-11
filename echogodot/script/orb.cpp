/* Jenova C++ Node Base Script (Meteora) */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN
bool orb_collected = false; 

void OnProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self || orb_collected) return;

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body && (body->get_name() == String("knight") || body->is_class("CharacterBody2D"))) {
			orb_collected = true; 
			Variant total = body->call("increase_inventory");
			UtilityFunctions::print("Orb Collected! Total: ", total);
			self->queue_free();
			break;
		}
	}
}
JENOVA_SCRIPT_END
