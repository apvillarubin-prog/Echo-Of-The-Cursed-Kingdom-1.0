extends Control

var dynamic_trigger_lock: bool = false

func _input(event: InputEvent) -> void:
	# Listens for any left-mouse click anywhere on the game window
	if event is InputEventMouseButton and event.is_pressed():
		if event.button_index == MOUSE_BUTTON_LEFT:
			trigger_revival()

func trigger_revival():
	# Prevents double-clicking inputs from breaking the engine transition mid-way
	if dynamic_trigger_lock: 
		return
	dynamic_trigger_lock = true
	
	# Fetch the path of the level we stored right before falling/dying
	var saved_level = TransitionManager.last_gameplay_level_path
	
	if saved_level != "":
		TransitionManager.fade_to_scene(saved_level)
	else:
		# Fallback just in case you test the scene directly without playing a level first
		print("No saved level found! Instantiating fallback scene.")
		TransitionManager.fade_to_scene("res://assetLvl3//state1.tscn")
