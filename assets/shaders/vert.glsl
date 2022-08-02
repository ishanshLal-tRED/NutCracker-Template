#version 450
#pragma shader_stage (vertex)

// input to vertex shader
// Vertex data
layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec2 v_TexCoord;

// Instance data
layout (location = 2) in vec4 i_model0;
layout (location = 3) in vec4 i_model1;
layout (location = 4) in vec4 i_model2;
layout (location = 5) in vec4 i_model3;
layout (location = 6) in vec2 i_TexStart;
layout (location = 7) in vec2 i_TexDelta;
layout (location = 8) in vec4 i_Color;
layout (location = 9) in int i_TexIndex;

// uniform buffer object
layout (binding = 0) uniform UniformBufferObject {
	mat4 ProjectionView;
} ubo;

// output to subsequent shader in subpass
layout (location = 0) out vec4 out_Color;
layout (location = 1) out vec3 out_UV;

void main () {
	mat4 model_mat = mat4 (i_model0, i_model1, i_model2, i_model3);
	gl_Position = ubo.ProjectionView * model_mat * vec4 (v_Position, 1.0);

	out_Color = i_Color;
	out_UV = vec3 (i_TexStart + v_TexCoord*i_TexDelta, i_TexIndex + 0.1);
}