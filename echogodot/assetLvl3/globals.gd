extends Node
signal player_health_change
var player_health=100:
	set(value):
		player_health=value
		player_health_change.emit()
var player_pos:Vector2
