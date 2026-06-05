#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/texture_rect.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/texture2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/utility_functions.hpp>
#include <Godot/variant/array.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Control* self = nullptr;
TextureRect* display = nullptr;
Button* btn_prev = nullptr;
Button* btn_next = nullptr;

int current_page = 0;
float current_slide_x = 0.0f;
float target_slide_x = 0.0f;
float base_y = 0.0f;

void update_slide(int direction) {
	if (!self || !self->has_meta("slidess")) return;

	Array slidess = self->get_meta("slidess");
	if (slidess.size() == 0) return;

	Ref<Texture2D> tex = slidess[current_page];
	if (display && tex.is_valid()) {
		display->set_texture(tex);
	}

	if (btn_prev) btn_prev->set_visible(current_page > 0);
	
	if (btn_next) {
		if (current_page == slidess.size() - 1) {
			btn_next->set_text("Finish");
		} else {
			btn_next->set_text("Next");
		}
	}

	if (display) {
		float screen_width = 1280.0f; 
		
		if (direction == 1) {
			current_slide_x = screen_width;
		} else if (direction == -1) { 
			current_slide_x = -screen_width;
		}
		
		display->set_position(Vector2(current_slide_x, base_y));
	}
}

void OnReady(Caller* instance) {
	self = GetSelf<Control>(instance);
	if (!self) return;

	self->set_visible(true);

	display = Object::cast_to<TextureRect>(self->get_node_or_null("SlideDisplay"));
	btn_prev = Object::cast_to<Button>(self->get_node_or_null("BtnPrev"));
	btn_next = Object::cast_to<Button>(self->get_node_or_null("BtnNext"));

	if (display) {
		target_slide_x = display->get_position().x;
		base_y = display->get_position().y;
		current_slide_x = target_slide_x;
	}

	if (self->get_tree()) {
		self->get_tree()->set_pause(true);
	}

	update_slide(0); 
}

void OnPhysicsProcess(Caller* instance, double delta) {
	if (!display) return;

	current_slide_x = UtilityFunctions::lerp(current_slide_x, target_slide_x, 12.0 * delta);
	display->set_position(Vector2(current_slide_x, base_y));
}

void _on_btn_prev_pressed() {
	if (current_page > 0) {
		current_page--;
		update_slide(-1); 
	}
}

void _on_btn_next_pressed() {
	if (self && self->has_meta("slidess")) {
		Array slidess = self->get_meta("slidess");
		if (current_page < slidess.size() - 1) {
			current_page++;
			update_slide(1);
		} else {
			if (self->get_tree()) {
				self->get_tree()->set_pause(false);
			}
			self->queue_free();
		}
	}
}

JENOVA_SCRIPT_END
