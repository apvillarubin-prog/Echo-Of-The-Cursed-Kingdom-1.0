/* Jenova C++ Node Base Script (Puzzle Lever) */
#include <Godot/godot.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/animated_sprite2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	Node2D* self = GetSelf<Node2D>(instance);
	if (!self) return;

	// Put this node into the 'lever' group automatically if it isn't already
	if (!self->is_in_group("lever")) self->add_to_group("lever");
	
	// Fallback tracker: make sure a default ID exists if not set in editor
	if (!self->has_meta("puzzle_id")) {
		self->set_meta("puzzle_id", 1);
	}
	self->set_meta("is_activated", false);

	// Initialize the sprite pointing left
	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (anim) {
		anim->play("left"); 
	}
}

void activate(Caller* instance) {
	Node2D* self = GetSelf<Node2D>(instance);
	if (!self) return;

	bool is_activated = self->has_meta("is_activated") ? (bool)self->get_meta("is_activated") : false;
	if (is_activated) return; // Ignores extra arrow hits once flipped
	
	self->set_meta("is_activated", true);
	int puzzle_id = (int)self->get_meta("puzzle_id");

	UtilityFunctions::print("[PUZZLE SYSTEM] Lever ID (", puzzle_id, ") flipped by Arrow!");

	// Play the rightward flip animation snap
	AnimatedSprite2D* anim = Object::cast_to<AnimatedSprite2D>(self->get_node_or_null("AnimatedSprite2D"));
	if (anim) {
		anim->play("right"); 
	}

	// Search the scene tree for the bridge matching this ID frequency
	TypedArray<Node> bridges = self->get_tree()->get_nodes_in_group("puzzle_bridge");
	for (int i = 0; i < bridges.size(); i++) {
		Node2D* bridge = Object::cast_to<Node2D>(bridges[i]);
		if (bridge) {
			int bridge_id = bridge->has_meta("puzzle_id") ? (int)bridge->get_meta("puzzle_id") : 1;
			
			if (bridge_id == puzzle_id) {
				bridge->call("trigger_bridge_appearance");
			}
		}
	}
}

JENOVA_SCRIPT_END
