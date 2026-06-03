shader_type canvas_item;

void fragment() {
    // Calculate distance from the edges of the zone
    vec2 border = smoothstep(vec2(0.0), vec2(0.15), UV) * smoothstep(vec2(0.0), vec2(0.15), vec2(1.0) - UV);
    float edge_fade = border.x * border.y;
    
    // Apply the fade strictly to the mask's visibility
    COLOR = vec4(1.0, 1.0, 1.0, edge_fade);
}