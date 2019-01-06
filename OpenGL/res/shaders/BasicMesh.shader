#shader vertex
#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 fragmentNormal;
out vec3 fragmentWorldPosition;

void main() {
	fragmentNormal = mat3(transpose(inverse(u_Model)))*normal;
	fragmentWorldPosition = vec3(u_Model * position);
	gl_Position = u_MVP * position;
};


#shader fragment
#version 330 core

in vec3 fragmentNormal;
in vec3 fragmentWorldPosition;

layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 color;

uniform vec3 u_LightColor;
uniform vec4 u_ObjectColor;
uniform vec3 u_LightPosition;

void main() {
	float ambientStrength = 0.25;
	vec3 ambient = ambientStrength * u_LightColor;

	float diff = max(dot(normalize(fragmentNormal), normalize(u_LightPosition - fragmentWorldPosition)), 0.0);
	vec3 diffuse = vec3(diff * u_LightColor);

	color = vec4((ambient + diffuse), 1.0) * u_ObjectColor;
};
