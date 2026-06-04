/* Jenova C++ Node Base Script */

#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/area2d.hpp>
#include <Godot/classes/collision_shape2d.hpp>

using namespace godot;
using namespace jenova::sdk;

Area2D* self = nullptr;
CollisionShape2D* wall_shape = nullptr;

JENOVA_SCRIPT_BEGIN

void _on_player_passed(Node* body)
{
	// Check if the colliding entity is our player
	if (body && body->is_in_group("player")) 
	{
		if (!wall_shape)
		{
			// Looks up and finds the shape inside BlockerWall
			wall_shape = self->get_node<CollisionShape2D>("../BlockerWall/CollisionShape2D");
		}

		if (wall_shape)
		{
			// Turn off 'Disabled' to instantly make the floor solid!
			wall_shape->set_deferred("disabled", false);
			
			// Delete the trigger zone so this code doesn't fire again
			self->queue_free();
		}
	}
}

void OnAwake(Caller* instance)
{
	self = GetSelf<Area2D>(instance);
}

JENOVA_SCRIPT_END
