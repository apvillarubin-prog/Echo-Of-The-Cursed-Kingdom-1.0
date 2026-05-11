/* Jenova C++ Node Base Script (Meteora) */
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

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	
	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body) {
			
			

			
			if (body->get_name() == String("knight") || body->is_class("CharacterBody2D")) {
				
				
				
				
				body->call("respawn");
				break; 
			}
		}
	}
}

JENOVA_SCRIPT_END
