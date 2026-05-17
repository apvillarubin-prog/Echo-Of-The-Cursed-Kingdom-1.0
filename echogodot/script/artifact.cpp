/* Jenova C++ Node Base Script (Meteora - Clean Artifact) */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (self) {
		self->set_meta("collected", false);
	}
}

void OnProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	bool collected = self->has_meta("collected") ? (bool)self->get_meta("collected") : false;
	if (collected) return;

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body && (body->get_name() == String("knight") || body->is_class("CharacterBody2D"))) {
			self->set_meta("collected", true);
			
			int total = (int)body->call("increase_inventory");
			UtilityFunctions::print("Artifact Collected! Current Level Inventory Count: ", total);
			
			self->queue_free();
			break;
		}
	}
}
JENOVA_SCRIPT_END
