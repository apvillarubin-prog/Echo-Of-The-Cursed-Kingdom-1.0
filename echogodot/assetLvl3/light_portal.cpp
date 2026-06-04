#include <Godot/godot.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/callable.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void on_body_entered(Caller* instance, Node2D* body) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self || !body) return;

	// Check if the object entering the portal is the player
	if (body->is_in_group("player")) {
		
		// Fetch the target path from the node's metadata
		String target_level_path = self->has_meta("target_level_path") ? (String)self->get_meta("target_level_path") : String("");

		if (!target_level_path.is_empty()) {
			// Disable the portal so it doesn't trigger twice while fading
			self->set_deferred("monitoring", false);
			
			// Call the global Autoload to handle the transition
			Node* transition_manager = self->get_node_or_null("/root/TransitionManager");
			if (transition_manager) {
				transition_manager->call("fade_to_scene", target_level_path);
			} else {
				// Fallback if TransitionManager isn't found
				UtilityFunctions::push_warning("[Portal] TransitionManager autoload not found! Falling back to standard scene change.");
				self->get_tree()->change_scene_to_file(target_level_path);
			}
		} else {
			UtilityFunctions::push_warning("[Portal] LightPortal has no 'target_level_path' metadata assigned!");
		}
	}
}

void OnReady(Caller* instance) {
	Area2D* self = GetSelf<Area2D>(instance);
	if (!self) return;

	// Connect the body_entered signal via code
	if (!self->is_connected("body_entered", Callable((Object*)self, "on_body_entered"))) {
		self->connect("body_entered", Callable((Object*)self, "on_body_entered"));
	}
}

JENOVA_SCRIPT_END
