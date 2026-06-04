extends Area2D

func _ready() -> void:
	# Automatically wire up the detection signal via code
	body_entered.connect(_on_body_entered)

func _on_body_entered(body: Node2D) -> void:
	# Ensures the object falling into the pit is actually your player character
	if body.is_in_group("player"):
		# Turn off the tracking area immediately so it doesn't fire multiple times
		set_deferred("monitoring", false)
		
		# Command the global manager to load the death scene layout
		TransitionManager.fade_to_scene("res://assetLvl3//DeathScreen.tscn") 
