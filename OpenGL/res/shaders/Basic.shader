#shader vertex
#version 330 core

layout(location = 0) in vec4 position;
layout(location = 2) in vec2 texCoord;

out vec2 v_TexCoord;

uniform mat4 u_MVP;
/* M1-3: Additional matrices for debugging */
uniform mat4 u_M1; 
uniform mat4 u_M2;
uniform mat4 u_M3;


void main() {
	gl_Position = u_MVP * u_M3 * u_M2 * u_M1 * position;
	v_TexCoord = texCoord;
};


#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

void main() {
	vec4 texColor = texture(u_Texture, v_TexCoord);
	color = texColor;
};
