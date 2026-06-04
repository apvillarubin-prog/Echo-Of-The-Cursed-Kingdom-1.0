/* Jenova C++ Node Base Script (Global) */
#include <Godot/godot.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance)
{
	Node* self = GetSelf<Node>(instance);
	if (!self) return;

	// Initialize the next_level metadata so it always exists by default
	if (!self->has_meta("next_level")) 
	{
		self->set_meta("next_level", 1);
	}

	UtilityFunctions::print("[GLOBAL] C++ Autoload Initialized.");
}

JENOVA_SCRIPT_END
