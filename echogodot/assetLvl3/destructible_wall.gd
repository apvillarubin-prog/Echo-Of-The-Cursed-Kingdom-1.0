extends StaticBody2D

# --- YOUR ORIGINAL VARIABLE ---
@export var hits_before_removance: int = 3

# Drag and drop your standalone crack PNGs here in the Inspector
@export var damage_textures: Array[Texture2D] = []

var current_hits: int = 0
var is_destroyed: bool = false

@onready var sprite: Sprite2D = get_node_or_null("Sprite2D")
@onready var collision_shape: CollisionShape2D = $CollisionShape2D
@onready var hit_audio: AudioStreamPlayer2D = get_node_or_null("HitAudio")

func _process(_delta: float) -> void:
	if is_destroyed:
		return

	# --- Knight Sword Registration ---
	if has_meta("pending_damage"):
		# Clear the meta but ignore the high damage number. 
		# This guarantees every swing counts as exactly 1 hit!
		remove_meta("pending_damage")
		register_hit()

func register_hit() -> void:
	if is_destroyed:
		return
		
	current_hits += 1
	
	# 1. Play hit sound instantly on every single strike
	if hit_audio:
		hit_audio.play()
		
	# 2. Flash the sprite white for a brief split-second
	if sprite:
		var flash_tween = create_tween()
		sprite.modulate = Color(4.0, 4.0, 4.0, 1.0) 
		flash_tween.tween_property(sprite, "modulate", Color.WHITE, 0.1)

	# 3. Check if the wall has taken too many hits
	if current_hits >= hits_before_removance:
		destroy_wall()
	else:
		# 4. SWAP THE IMAGE PER HIT:
		# Hit 1 loads damage_textures[0]
		# Hit 2 loads damage_textures[1]...
		var texture_index = current_hits - 1
		
		if sprite and texture_index >= 0 and texture_index < damage_textures.size():
			if damage_textures[texture_index] != null:
				sprite.texture = damage_textures[texture_index]

func destroy_wall() -> void:
	is_destroyed = true
	
	# Disable collisions instantly so the player doesn't hit a wall while it's disappearing
	if collision_shape:
		collision_shape.set_deferred("disabled", true)
		
	# Hide the artwork
	if sprite:
		sprite.visible = false
		
	# Let the final breaking sound finish playing fully
	if hit_audio and hit_audio.playing:
		await hit_audio.finished
		
	queue_free()
