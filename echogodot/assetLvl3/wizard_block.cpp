#include <Godot/godot.hpp>
#include <Godot/classes/rigid_body2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/scene_tree_timer.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

RigidBody2D* block = nullptr;
float lifetime = 10.0f;

void OnReady(Caller* instance) {
	block = GetSelf<RigidBody2D>(instance);
	if (block) {
		// FIXED: Godot 4 uses set_lock_rotation_enabled instead of set_lock_rotation
		block->set_lock_rotation_enabled(true); 
		block->set_mass(1.0f);
	}
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!block) return;

	lifetime -= (float)delta;
	if (lifetime <= 0.0f) {
		block->queue_free(); // Safely delete the block
	}
}

JENOVA_SCRIPT_END
