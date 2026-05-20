// Attach this to your PointLight2D node
#include <Godot/godot.hpp>
#include <Godot/classes/point_light2d.hpp>
#include <Godot/variant/utility_functions.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

void OnPhysicsProcess(Caller* instance, double delta) {
	PointLight2D* light = GetSelf<PointLight2D>(instance);
	if (!light) return;
	
	// Add a tiny bit of random noise to the energy
	float noise = (float)UtilityFunctions::randf_range(-0.1, 0.1);
	light->set_energy(1.2f + noise);
}

JENOVA_SCRIPT_END
