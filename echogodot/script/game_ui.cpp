#include <Godot/godot.hpp>
#include <Godot/classes/canvas_layer.hpp>
#include <Godot/classes/progress_bar.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/node.hpp>
#include <Godot/classes/input.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/callable.hpp>
#include <unordered_map>
#include <memory>
using namespace godot;
using namespace jenova::sdk;
class UIManager {
public:
	CanvasLayer* self = nullptr;
	ProgressBar* health_bar = nullptr;
	Control* death_screen = nullptr;
	Button* respawn_btn = nullptr;
	void on_ready(CanvasLayer* node) {
		self = node;
		if (!self->is_in_group("game_ui")) {
			self->add_to_group("game_ui", true);
		}
		health_bar = Object::cast_to<ProgressBar>(self->get_node_or_null("HealthBar"));
		death_screen = Object::cast_to<Control>(self->get_node_or_null("DeathScreen"));
		if (health_bar) health_bar->set_visible(false);
		if (death_screen) {
			death_screen->set_visible(false);
			respawn_btn = Object::cast_to<Button>(death_screen->get_node_or_null("RespawnButton"));
			if (respawn_btn) {
				respawn_btn->set_focus_mode(Control::FOCUS_ALL);
				if (!respawn_btn->is_connected("pressed", Callable((Object*)self, "trigger_respawn"))) {
					respawn_btn->connect("pressed", Callable((Object*)self, "trigger_respawn"));
				}
			}
			UtilityFunctions::print("[UI] GameUI Ready and listening.");
		}
	}
	void show_ui() {
		if (health_bar) health_bar->set_visible(true);
	}
	void hide_ui() {
		if (health_bar) health_bar->set_visible(false);
	}
	void update_health(int current_health) {
		if (health_bar) health_bar->set_value((double)current_health);
	}
	// NEW: reset health bar back to max value
	void reset_health_bar() {
		if (health_bar) {
			health_bar->set_value(health_bar->get_max());
			UtilityFunctions::print("[UI] Health bar reset to max.");
		}
	}
	void show_death_screen() {
		if (death_screen) {
			death_screen->set_visible(true);
			Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
		}
	}
	void trigger_respawn() {
		if (death_screen) death_screen->set_visible(false);
		Node* player = self->get_tree()->get_first_node_in_group("player");
		if (player) {
			player->call("reset_health");
			player->call("actually_teleport");
		}
	}
	void on_exit_tree() {
		UtilityFunctions::print("[UI-FATAL] ALARM! The GameUI Autoload was just deleted from the SceneTree!");
	}
};
static std::unordered_map<uint64_t, std::shared_ptr<UIManager>> ui_instances;
std::shared_ptr<UIManager> get_ui(Caller* instance) {
	if (!instance) return nullptr;
	CanvasLayer* node = GetSelf<CanvasLayer>(instance);
	if (!node) return nullptr;
	uint64_t id = node->get_instance_id();
	if (ui_instances.find(id) == ui_instances.end()) ui_instances[id] = std::make_shared<UIManager>();
	return ui_instances[id];
}
JENOVA_SCRIPT_BEGIN
void OnReady(Caller* instance) { if (auto ui = get_ui(instance)) ui->on_ready(GetSelf<CanvasLayer>(instance)); }
void show_ui(Caller* instance) { if (auto ui = get_ui(instance)) ui->show_ui(); }
void hide_ui(Caller* instance) { if (auto ui = get_ui(instance)) ui->hide_ui(); }
void update_health(Caller* instance, int current_health) { if (auto ui = get_ui(instance)) ui->update_health(current_health); }
void reset_health_bar(Caller* instance) { if (auto ui = get_ui(instance)) ui->reset_health_bar(); }
void show_death_screen(Caller* instance) { if (auto ui = get_ui(instance)) ui->show_death_screen(); }
void trigger_respawn(Caller* instance) { if (auto ui = get_ui(instance)) ui->trigger_respawn(); }
void OnExitTree(Caller* instance) { if (auto ui = get_ui(instance)) ui->on_exit_tree(); }
JENOVA_SCRIPT_END
