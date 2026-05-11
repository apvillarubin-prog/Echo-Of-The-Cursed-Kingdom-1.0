/* Example for Orb/Gem/Artifact */
#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN
bool is_taken = false; 

void OnProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self || is_taken) return;

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body && (body->get_name() == String("knight") || body->is_class("CharacterBody2D"))) {
			is_taken = true; 
			body->call("increase_inventory");
			self->queue_free();
			break;
		}
	}
}
JENOVA_SCRIPT_END
