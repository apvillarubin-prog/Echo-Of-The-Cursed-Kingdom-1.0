/* Jenova C++ Label Base Script (Meteora) */

// Godot SDK
#include <Godot/godot.hpp>
#include <Godot/classes/label.hpp>

// Namespaces
using namespace godot;
using namespace jenova::sdk;

// Safe primitive types are allowed globally
static float timer = 0.0f;
static int dot_count = 0;
static float update_speed = 0.4f;

// Jenova Script Block Start
JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance)
{
	Label* self = GetSelf<Label>(instance);
	if (self) 
	{
		// Moved the Godot String type inside the function scope to follow Godot design
		String base_text = "Loading"; 
		self->set_text(base_text);
		timer = 0.0f;
		dot_count = 0;
	}
}

void OnProcess(Caller* instance, double delta)
{
	Label* self = GetSelf<Label>(instance);
	if (!self) return;

	timer += static_cast<float>(delta);
	
	if (timer >= update_speed) 
	{
		timer = 0.0f;
		dot_count = (dot_count + 1) % 4; // Cycles smoothly: 0, 1, 2, 3 dots
		
		String base_text = "Loading";
		String dots = "";
		for (int i = 0; i < dot_count; i++) 
		{
			dots += ".";
		}
		
		self->set_text(base_text + dots);
	}
}

// Jenova Script Block End
JENOVA_SCRIPT_END
