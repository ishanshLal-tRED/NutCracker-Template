#version 450
#pragma shader_stage (fragment)

// input to fragment shader
layout (location = 0) in vec4 in_Color;
layout (location = 1) in vec3 in_UV;

layout (location = 0) out vec4 out_Color;

layout (binding = 1) uniform sampler2D textureSampler[2];

void main () {
	out_Color = in_Color;
}