#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/label.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

// Notice we removed the global 'instruction_label = nullptr' from up here.
// This prevents InstructionZone2 from overwriting InstructionZone1's label.

void OnReady(Caller* instance) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Fetch the Label that belongs specifically to THIS Area2D instance
	Label* instruction_label = Object::cast_to<Label>(self->get_node_or_null("InstructionText"));

	if (instruction_label) {
		instruction_label->set_visible(false); 
	} else {
		UtilityFunctions::print("[DEBUG-ZONE] Error: Could not find InstructionText label on ", self->get_name());
	}
}

void OnProcess(Caller* instance, double delta) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Fetch the Label for this specific zone every frame
	Label* instruction_label = Object::cast_to<Label>(self->get_node_or_null("InstructionText"));
	if (!instruction_label) return;

	TypedArray<Node2D> bodies = self->get_overlapping_bodies();
	bool player_in_zone = false;

	for (int i = 0; i < bodies.size(); i++) {
		Node2D* body = Object::cast_to<Node2D>(bodies[i]);
		if (body && (body->get_name() == String("knight") || body->is_in_group("player"))) {
			player_in_zone = true;
			break; 
		}
	}

	// Show/hide only THIS zone's label
	instruction_label->set_visible(player_in_zone);
}

JENOVA_SCRIPT_END
