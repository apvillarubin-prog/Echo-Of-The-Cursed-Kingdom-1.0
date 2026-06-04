#include <Godot/godot.hpp>
#include <Godot/classes/canvas_layer.hpp>
#include <Godot/classes/color_rect.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/classes/scene_tree_timer.hpp>
#include <Godot/classes/tween.hpp>
#include <Godot/classes/property_tweener.hpp>
#include <Godot/classes/callback_tweener.hpp>
#include <Godot/classes/node2d.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/callable.hpp>
#include <unordered_map>
#include <memory>

using namespace godot;
using namespace jenova::sdk;

class TransitionManager {
public:
	CanvasLayer* self = nullptr;
	ColorRect* color_rect = nullptr;
	
	float fade_time = 0.5f;
	String last_gameplay_level_path = "";
	String pending_scene_path = ""; // Stores the path mid-transition

	void on_ready(CanvasLayer* node) {
		self = node;
		color_rect = Object::cast_to<ColorRect>(self->get_node_or_null("ColorRect"));
		
		if (!color_rect) {
			UtilityFunctions::push_error("[TransitionManager] ColorRect child not found!");
		}
	}

	void fade_to_scene(String scene_path) {
		if (!self || !color_rect) return;

		pending_scene_path = scene_path;
		String lower_path = scene_path.to_lower();

		// If heading to the death screen, save the current level's path
		if (lower_path.find("death") != -1) {
			Node* current_scene = self->get_tree()->get_current_scene();
			if (current_scene) {
				last_gameplay_level_path = current_scene->get_scene_file_path();
			}
		}

		// 1. Fade to Black
		Ref<Tween> fade_out_tween = self->create_tween();
		if (fade_out_tween.is_valid()) {
			fade_out_tween->tween_property(color_rect, "color:a", 1.0f, fade_time);
			// Queue Step 2 when the tween finishes
			fade_out_tween->tween_callback(Callable((Object*)self, "step2_change_scene"));
		}
	}

	void step2_change_scene() {
		// 2. Switch the Game Scene File
		self->get_tree()->change_scene_to_file(pending_scene_path);

		// 3. Buffer Timer (Replaces `await get_tree().create_timer(0.05).timeout`)
		Ref<SceneTreeTimer> buffer_timer = self->get_tree()->create_timer(0.05f);
		if (buffer_timer.is_valid()) {
			// Queue Step 3 when the timer finishes
			buffer_timer->connect("timeout", Callable((Object*)self, "step3_finish_fade"));
		}
	}

	void step3_finish_fade() {
		String lower_path = pending_scene_path.to_lower();

		// 4. Position the player if NOT going to the death screen
		if (lower_path.find("death") == -1) {
			position_player_at_spawn();
		}

		// 5. Fade back to Transparent
		Ref<Tween> fade_in_tween = self->create_tween();
		if (fade_in_tween.is_valid()) {
			fade_in_tween->tween_property(color_rect, "color:a", 0.0f, fade_time);
		}
	}

	void position_player_at_spawn() {
		Node* current_scene = self->get_tree()->get_current_scene();
		if (!current_scene) {
			UtilityFunctions::print("System Error: Scene failed to initialize safely. Aborting spawn alignment.");
			return;
		}

		Node* player_node = self->get_tree()->get_first_node_in_group("player");
		Node2D* spawn_point = Object::cast_to<Node2D>(current_scene->get_node_or_null("SpawnPoint"));

		if (player_node && spawn_point) {
			Node2D* player_2d = Object::cast_to<Node2D>(player_node);
			if (player_2d) {
				player_2d->set_global_position(spawn_point->get_global_position());
			}
		} else if (player_node && !spawn_point) {
			UtilityFunctions::print("System Warning: Level loaded but no 'SpawnPoint' node was found!");
		}
	}
};

// ============================================================================
// INSTANCE MAPPER & JENOVA BINDINGS
// ============================================================================

static std::unordered_map<uint64_t, std::shared_ptr<TransitionManager>> tm_instances;

std::shared_ptr<TransitionManager> get_tm(Caller* instance) {
	if (!instance) return nullptr;
	CanvasLayer* node = GetSelf<CanvasLayer>(instance);
	if (!node) return nullptr;

	uint64_t id = node->get_instance_id();
	if (tm_instances.find(id) == tm_instances.end()) {
		tm_instances[id] = std::make_shared<TransitionManager>();
	}
	return tm_instances[id];
}

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	if (auto tm = get_tm(instance)) tm->on_ready(GetSelf<CanvasLayer>(instance));
}

void fade_to_scene(Caller* instance, String scene_path) {
	if (auto tm = get_tm(instance)) tm->fade_to_scene(scene_path);
}

// Internal sequence callbacks
void step2_change_scene(Caller* instance) {
	if (auto tm = get_tm(instance)) tm->step2_change_scene();
}

void step3_finish_fade(Caller* instance) {
	if (auto tm = get_tm(instance)) tm->step3_finish_fade();
}

void position_player_at_spawn(Caller* instance) {
	if (auto tm = get_tm(instance)) tm->position_player_at_spawn();
}

JENOVA_SCRIPT_END
