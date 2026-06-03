extends CanvasLayer

@onready var color_rect = $ColorRect
var fade_time: float = 0.5 

# Remembers the level file path right before the player dies
var last_gameplay_level_path: String = "" 

func fade_to_scene(scene_path: String):
	# If we are heading to the death screen, save the current level's path first
	if "death" in scene_path.to_lower():
		last_gameplay_level_path = get_tree().current_scene.scene_file_path

	# 1. Fade to Black
	var fade_out_tween = create_tween()
	fade_out_tween.tween_property(color_rect, "color:a", 1.0, fade_time)
	await fade_out_tween.finished
	
	# 2. Switch the Game Scene File
	get_tree().change_scene_to_file(scene_path)
	
	# 3. Wait exactly one frame for Godot to fully build the new level's nodes
	await get_tree().process_frame
	
	# 4. If we are entering a normal level (NOT the death screen), position the player
	if not "death" in scene_path.to_lower():
		position_player_at_spawn()
	
	# 5. Fade back to Transparent
	var fade_in_tween = create_tween()
	fade_in_tween.tween_property(color_rect, "color:a", 0.0, fade_time)

func position_player_at_spawn():
	# 1. SAFETY CHECK: Get the scene and ensure it isn't null
	var current_scene = get_tree().current_scene
	if current_scene == null:
		print("System Error: Scene failed to initialize safely. Aborting spawn alignment.")
		return

	# 2. Find your C++ player character via its group
	var player = get_tree().get_first_node_in_group("player")
	
	# 3. Now it is 100% safe to search for the SpawnPoint node
	var spawn_point = current_scene.get_node_or_null("SpawnPoint")
	
	if player and spawn_point:
		player.global_position = spawn_point.global_position
	elif player and not spawn_point:
		print("System Warning: Level loaded but no 'SpawnPoint' node was found!")
