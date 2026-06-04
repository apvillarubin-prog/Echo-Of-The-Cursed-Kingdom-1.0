extends CharacterBody2D

# Config constants
const AGGRO_RANGE: float = 200.0
const CHASE_SPEED: float = 40.0
const PATROL_SPEED: float = 25.0
const GRAVITY: float = 1000.0

# --- NEW COMBAT CONSTANTS ---
const KNOCKBACK_FORCE: float = 180.0  # Horizontal speed of the shove
const KNOCKBACK_LIFT: float = -120.0   # Vertical hop to break ground friction

@export var max_aggro_y: float = 50.0 
@export var attack_range: float = 30.0
@export var max_health: int = 40

var current_health: int = 15
var is_dying: bool = false
var attack_cooldown: float = 0.0
var last_hit_frame: int = -1
var patrol_direction: int = 1 


# --- NEW COMBAT TRACKERS ---
var knockback_timer: float = 0.0

@onready var anim: AnimatedSprite2D = $AnimatedSprite2D
@onready var vision_ray: RayCast2D = get_node_or_null("RayCast2D")
@onready var hit_audio: AudioStreamPlayer2D = get_node_or_null("HitAudio") # Crash-proof audio ref

func _ready() -> void:
	if not is_in_group("enemy"):
		add_to_group("enemy")
	set_collision_mask_value(2, false)
	current_health = max_health

func _physics_process(delta: float) -> void:
	if is_dying:
		return

	# --- NEW: Knockback Stun State Override ---
	# If the enemy was recently struck, count down the timer, apply gravity, and skip AI logic
	if knockback_timer > 0.0:
		knockback_timer -= delta
		if not is_on_floor():
			velocity.y += GRAVITY * delta
		move_and_slide()
		return # Prevents patrol or chase code from overriding the knockback speed

	# --- Damage Registration ---
	if has_meta("pending_damage"):
		var incoming_dmg: int = get_meta("pending_damage")
		remove_meta("pending_damage")
		current_health -= incoming_dmg
		
		if current_health <= 0:
			die()
			return
		else:
			# 1. Play the hit sound effect
			if hit_audio:
				hit_audio.play()
			
			# 2. Calculate direction away from the player
			var players = get_tree().get_nodes_in_group("player")
			if players.size() > 0:
				var player = players[0]
				# If enemy is to the right of player, fly right (1). Otherwise fly left (-1).
				var push_dir = 1 if global_position.x > player.global_position.x else -1
				
				# Apply forces
				velocity.x = push_dir * KNOCKBACK_FORCE
				velocity.y = KNOCKBACK_LIFT 
				knockback_timer = 0.15 # Stun lasts for 150 milliseconds
				
				# Flash a quick visual indicator or play a hit state frame
				anim.play("enemy_idle") 

	if attack_cooldown > 0.0:
		attack_cooldown -= delta

	if not is_on_floor():
		velocity.y += GRAVITY * delta

	# --- Target Tracking & Line of Sight ---
	var target: Node2D = null
	var players = get_tree().get_nodes_in_group("player")
	if players.size() > 0:
		target = players[0]

	var can_see_player: bool = false

	if target:
		var p_pos: Vector2 = target.global_position
		var e_pos: Vector2 = global_position
		
		var dist_x: float = abs(p_pos.x - e_pos.x)
		var dist_y: float = abs(p_pos.y - e_pos.y)

		if dist_x <= AGGRO_RANGE and dist_y <= max_aggro_y:
			if vision_ray:
				vision_ray.target_position = vision_ray.to_local(p_pos)
				vision_ray.force_raycast_update()
				
				if vision_ray.is_colliding():
					var collider = vision_ray.get_collider()
					if collider and collider.is_in_group("player"):
						can_see_player = true
					else:
						can_see_player = false
				else:
					can_see_player = true
			else:
				can_see_player = true

		# ==========================================
		# AGGRO STATE
		# ==========================================
		if can_see_player:
			var dir: int = 1 if p_pos.x > e_pos.x else -1
			
			if attack_cooldown <= 0.0 or anim.animation != "enemy_attack":
				anim.flip_h = (dir < 0)

			if dist_x > attack_range and attack_cooldown <= 0.0:
				velocity.x = dir * CHASE_SPEED
				anim.play("enemy_run")
				last_hit_frame = -1
			else:
				velocity.x = 0 
				if attack_cooldown <= 0.0:
					anim.play("enemy_attack")

				if anim.animation == "enemy_attack":
					var current_frame: int = anim.frame
					if current_frame == 2 and last_hit_frame != 2:
						if dist_x <= attack_range + 10.0 and dist_y <= max_aggro_y:
							if target.has_method("take_damage"):
								target.call("take_damage", 1)
						last_hit_frame = 2
						attack_cooldown = 1.5 
					elif current_frame != 2:
						last_hit_frame = -1
				elif attack_cooldown > 0.0:
					anim.play("enemy_idle")
					
		# ==========================================
		# PATROL STATE
		# ==========================================
		else:
			velocity.x = patrol_direction * PATROL_SPEED
			anim.play("enemy_run")
			anim.flip_h = (patrol_direction < 0)
			last_hit_frame = -1
	else:
		velocity.x = patrol_direction * PATROL_SPEED
		anim.play("enemy_run")
		anim.flip_h = (patrol_direction < 0)

	move_and_slide()

	# Wall Turnaround
	if is_on_wall():
		if not can_see_player:
			patrol_direction *= -1

# --- JUICY DEATH FUNCTION ---
func die() -> void:
	is_dying = true
	
	set_physics_process(false)
	velocity = Vector2.ZERO
	
	var collision_shape = get_node_or_null("CollisionShape2D")
	if collision_shape:
		collision_shape.set_deferred("disabled", true)
		
	anim.modulate = Color(5.0, 0.4, 0.4, 1.0) 
	
	var death_tween = create_tween().set_parallel(true)
	var duration: float = 0.35
	
	var spin_angle = randf_range(-6.0, 6.0)
	death_tween.tween_property(self, "rotation", spin_angle, duration)
	death_tween.tween_property(self, "scale", Vector2.ZERO, duration).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_IN)
	death_tween.tween_property(anim, "modulate:a", 0.0, duration)
	
	death_tween.chain().tween_callback(queue_free)
	
func freeze_enemy():
	# 1. STOP ALL MOVEMENT IMMEDIATELY
	if self is CharacterBody2D:
		velocity = Vector2.ZERO  # zero out velocity

	# 2. KILL THE CODE
	set_physics_process(false)
	set_process(false)

	# 3. FREEZE THE VISUALS
	var sprite: AnimatedSprite2D = $AnimatedSprite2D
	if sprite:
		sprite.stop()  # stop animation
		sprite.modulate = Color(0.5, 0.7, 1.0, 1.0)  # tint blue
