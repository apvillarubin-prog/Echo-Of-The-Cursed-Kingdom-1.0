#include <Godot/godot.hpp>
#include <Godot/classes/control.hpp>
#include <Godot/classes/texture_rect.hpp>
#include <Godot/classes/button.hpp>
#include <Godot/classes/texture2d.hpp>
#include <Godot/classes/scene_tree.hpp>
#include <Godot/variant/array.hpp>

using namespace godot;
using namespace jenova::sdk;

JENOVA_SCRIPT_BEGIN

Control* self = nullptr;
TextureRect* display = nullptr;
Button* btn_next = nullptr;
int current_page = 0;

void OnReady(Caller* instance) {
	self = GetSelf<Control>(instance);
	if (!self) return;

	display = Object::cast_to<TextureRect>(self->get_node_or_null("SlideDisplay"));
	btn_next = Object::cast_to<Button>(self->get_node_or_null("BtnNext"));

	if (self->has_meta("slides")) {
		Array slides = self->get_meta("slides");
		if (slides.size() > 0 && display) {
			display->set_texture(slides[0]);
		}
		if (slides.size() == 1 && btn_next) {
			btn_next->set_text("Done");
		}
	}
}

void _on_btn_next_pressed() {
	if (!self || !self->has_meta("slides")) return;

	Array slides = self->get_meta("slides");
	current_page++;

	if (current_page < slides.size()) {
		if (display) {
			display->set_texture(slides[current_page]);
		}
		if (btn_next && current_page == slides.size() - 1) {
			btn_next->set_text("Done");
		}
	} else {
		if (self->get_tree()) {
			self->get_tree()->change_scene_to_file("res://scene//loading_screen.tscn");
		}
	}
}

JENOVA_SCRIPT_END
