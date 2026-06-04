/* Jenova C++ Node Base Script (Meteora) */

// Godot SDK
#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/variant/variant.hpp>

// Namespaces
using namespace godot;
using namespace jenova::sdk;

// Self Instance
Area2D* self = nullptr;

// Jenova Script Block Start
JENOVA_SCRIPT_BEGIN

// Signals / Callbacks
void _on_body_entered(Node* body)
{
	// Check if the entering node belongs to the "player" group
	if (body && body->is_in_group("player")) 
	{
		Node* parent = self->get_parent();
		if (parent)
		{
			Color current_color = parent->get("modulate");
			current_color.a = 0.1f;
			parent->set("modulate", current_color);
		}
	}
}

void _on_body_exited(Node* body)
{
	if (body && body->is_in_group("player")) 
	{
		Node* parent = self->get_parent();
		if (parent)
		{
			Color current_color = parent->get("modulate");
			current_color.a = 1.0f;
			parent->set("modulate", current_color);
		}
	}
}


// Routines
void OnAwake(Caller* instance)
{
	// Called When Node Enters Scene Tree
	self = GetSelf<Area2D>(instance);
}
void OnDestroy(Caller* instance)
{
	// Called When Node Exits Scene Tree
	self = nullptr;
}
void OnReady(Caller* instance)
{
	// Called When Node and All It's Children Entered Scene Tree
}
void OnProcess(Caller* instance, double _delta)
{
	// Called On Every Frame
}

// Jenova Script Block End
JENOVA_SCRIPT_END
