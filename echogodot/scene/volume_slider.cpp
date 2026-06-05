#include <Godot/godot.hpp>
#include <Godot/classes/h_slider.hpp>
#include <Godot/classes/audio_server.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/callable.hpp>
#include <unordered_map>
#include <memory>

using namespace godot;
using namespace jenova::sdk;

class VolumeController {
public:
	HSlider* self = nullptr;
	int master_bus_index = 0;

	void on_ready(HSlider* node) {
		self = node;
		
		// Get the Master audio bus ID
		master_bus_index = AudioServer::get_singleton()->get_bus_index("Master");

		// Dynamically connect the slider movement to our C++ function
		if (!self->is_connected("value_changed", Callable((Object*)self, "change_volume"))) {
			self->connect("value_changed", Callable((Object*)self, "change_volume"));
		}

		// Force the audio to match the slider's initial 1.0 (max) value right away
		change_volume(self->get_value());
		
		UtilityFunctions::print("[UI] Clean Volume Slider Ready!");
	}

	void change_volume(double value) {
		// Convert the 0.0 to 1.0 value into Godot's Decibel system
		double db_volume = UtilityFunctions::linear_to_db(value);
		AudioServer::get_singleton()->set_bus_volume_db(master_bus_index, db_volume);
		
		// Mute completely if it's dragged all the way to the left
		AudioServer::get_singleton()->set_bus_mute(master_bus_index, value <= 0.001);
	}
};

static std::unordered_map<uint64_t, std::shared_ptr<VolumeController>> volume_instances;

std::shared_ptr<VolumeController> get_volume_controller(Caller* instance) {
	if (!instance) return nullptr;
	HSlider* node = GetSelf<HSlider>(instance);
	if (!node) return nullptr;
	uint64_t id = node->get_instance_id();
	if (volume_instances.find(id) == volume_instances.end()) volume_instances[id] = std::make_shared<VolumeController>();
	return volume_instances[id];
}

JENOVA_SCRIPT_BEGIN
void OnReady(Caller* instance) { if (auto ctrl = get_volume_controller(instance)) ctrl->on_ready(GetSelf<HSlider>(instance)); }
void change_volume(Caller* instance, double value) { if (auto ctrl = get_volume_controller(instance)) ctrl->change_volume(value); }
JENOVA_SCRIPT_END
