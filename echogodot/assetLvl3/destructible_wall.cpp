#include <Godot/godot.hpp>
#include <Godot/classes/static_body2d.hpp>
#include <Godot/classes/sprite2d.hpp>
#include <Godot/classes/collision_shape2d.hpp>
#include <Godot/classes/audio_stream_player2d.hpp>
#include <Godot/classes/tween.hpp>
#include <Godot/classes/property_tweener.hpp>
#include <Godot/classes/resource_loader.hpp>
#include <Godot/classes/texture2d.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/string.hpp>
#include <Godot/variant/color.hpp>
#include <Godot/variant/callable.hpp>
#include <unordered_map>
#include <memory>

using namespace godot;
using namespace jenova::sdk;

class WallController {
public:
	StaticBody2D* self = nullptr;
	Sprite2D* sprite = nullptr;
	CollisionShape2D* collision_shape = nullptr;
	AudioStreamPlayer2D* hit_audio = nullptr;

	int hits_before_removance = 3;
	int current_hits = 0;
	bool is_destroyed = false;

	void on_ready(StaticBody2D* node) {
		self = node;
		sprite = Object::cast_to<Sprite2D>(self->get_node_or_null("Sprite2D"));
		collision_shape = Object::cast_to<CollisionShape2D>(self->get_node_or_null("CollisionShape2D"));
		hit_audio = Object::cast_to<AudioStreamPlayer2D>(self->get_node_or_null("HitAudio"));

		// Load custom structural hit count if provided via Inspector metadata
		if (self->has_meta("hits_before_removance")) {
			hits_before_removance = (int)self->get_meta("hits_before_removance");
		}
		
		current_hits = 0;
		is_destroyed = false;
	}

	void register_hit() {
		if (is_destroyed) return;

		current_hits++;

		// 1. Play hit sound instantly
		if (hit_audio) hit_audio->play();

		// 2. Flash the sprite white for a split second
		if (sprite) {
			sprite->set_modulate(Color(4.0f, 4.0f, 4.0f, 1.0f)); // Over-brighten values
			Ref<Tween> flash_tween = self->create_tween();
			if (flash_tween.is_valid()) {
				flash_tween->tween_property(sprite, "modulate", Color(1.0f, 1.0f, 1.0f, 1.0f), 0.1f);
			}
		}

		// 3. Check structural damage limits
		if (current_hits >= hits_before_removance) {
			destroy_wall();
		} else {
			// 4. Swap cracks texturing stage via dynamic metadata keys
			String meta_key = String("crack_texture_") + String::num_int64(current_hits);
			if (self->has_meta(meta_key)) {
				String texture_path = (String)self->get_meta(meta_key);
				if (!texture_path.is_empty()) {
					Ref<Texture2D> next_texture = ResourceLoader::get_singleton()->load(texture_path);
					if (next_texture.is_valid() && sprite) {
						sprite->set_texture(next_texture);
					}
				}
			}
		}
	}

	void destroy_wall() {
		is_destroyed = true;

		// Disable collision instantly so the player falls through right away
		if (collision_shape) collision_shape->set_deferred("disabled", true);

		// Hide artwork visibility
		if (sprite) sprite->set_visible(false);

		// Safely finish playback before removing the node instance
		if (hit_audio && hit_audio->is_playing()) {
			hit_audio->connect("finished", Callable(self, "queue_free"));
		} else {
			self->queue_free();
		}
	}

	void on_process(double delta) {
		if (is_destroyed) return;

		// --- Knight Sword Registration ---
		if (self->has_meta("pending_damage")) {
			self->remove_meta("pending_damage");
			register_hit();
		}
	}
};

// ============================================================================
// INSTANCE MAPPER & JENOVA BINDINGS
// ============================================================================

static std::unordered_map<uint64_t, std::shared_ptr<WallController>> wall_instances;

std::shared_ptr<WallController> get_wall(Caller* instance) {
	if (!instance) return nullptr;
	StaticBody2D* node = GetSelf<StaticBody2D>(instance);
	if (!node) return nullptr;

	uint64_t id = node->get_instance_id();
	if (wall_instances.find(id) == wall_instances.end()) {
		wall_instances[id] = std::make_shared<WallController>();
	}
	return wall_instances[id];
}

JENOVA_SCRIPT_BEGIN

void OnReady(Caller* instance) {
	if (auto w = get_wall(instance)) w->on_ready(GetSelf<StaticBody2D>(instance));
}

void OnProcess(Caller* instance, double delta) {
	if (auto w = get_wall(instance)) w->on_process(delta);
}

JENOVA_SCRIPT_END
