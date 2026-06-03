shader_type canvas_item;

// Exposed variables for tweaking in the Inspector
uniform vec4 fog_color : source_color = vec4(0.6, 0.65, 0.7, 0.7); // High visibility default
uniform sampler2D noise_texture : repeat_enable;
uniform vec2 scroll_speed = vec2(0.04, 0.01);
uniform float density : hint_range(0.0, 1.0) = 0.6; // Higher density default

void fragment() {
    // 1. Calculate the rolling fog noise
    vec2 move1 = UV + TIME * scroll_speed;
    vec2 move2 = UV - TIME * (scroll_speed * 0.6) + vec2(0.2, 0.5);
    
    float noise1 = texture(noise_texture, move1).r;
    float noise2 = texture(noise_texture, move2).r;
    float final_noise = (noise1 + noise2) / 2.0;
    
    float alpha_mask = smoothstep(1.0 - density, 1.0, final_noise);
    
    // 2. Calculate the soft feathered edges (0.15 controls how wide the fade is)
    vec2 border = smoothstep(vec2(0.0), vec2(0.15), UV) * smoothstep(vec2(0.0), vec2(0.15), vec2(1.0) - UV);
    float edge_fade = border.x * border.y;
    
    // 3. Combine them! 
    vec4 color = fog_color;
    color.a *= alpha_mask * edge_fade; // Noise multiplied by the edge fade
    
    COLOR = color;
}