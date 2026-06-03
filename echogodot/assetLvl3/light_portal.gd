extends Area2D

# This allows you to pick the target scene directly in the Godot Inspector!
@export_file("*.tscn") var target_level_path: String

func _ready():
	# Connect the body_entered signal via code so you don't have to do it manually in the editor
	body_entered.connect(_on_body_entered)

func _on_body_entered(body: Node2D):
	# Check if the object entering the portal is your player
	# (Your C++ code already adds the player to the "player" group, so this works perfectly!)
	if body.is_in_group("player"):
		if target_level_path != "":
			# Disable the portal so it doesn't trigger twice while fading
			set_deferred("monitoring", false)
			
			# Call our global Autoload to handle the transition
			TransitionManager.fade_to_scene(target_level_path)
		else:
			push_warning("LightPortal has no target scene assigned!")
