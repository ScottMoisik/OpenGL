#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

out vec2 v_TexCoord;
out vec3 fragmentNormal;
out vec3 fragmentWorldPosition;

void main() {
	vec4 posHomo = vec4(position, 1.0);
	fragmentNormal = mat3(transpose(inverse(u_Model)))*normal;
	fragmentWorldPosition = vec3(u_Model * posHomo);
	gl_Position = u_Proj * u_View * u_Model * posHomo;
	v_TexCoord = texCoord;
};


#shader fragment
#version 330 core

in vec2 v_TexCoord;

in vec3 fragmentNormal;
in vec3 fragmentWorldPosition;

layout(location = 0) out vec4 color;

uniform int u_UseTexturing;
uniform vec3 u_LightColor;
uniform vec4 u_ObjectColor;
uniform vec3 u_LightPosition;
uniform sampler2D u_Texture;

void main() {
	vec4 objColor = u_ObjectColor;

	if (bool(u_UseTexturing)) {
		objColor = texture(u_Texture, v_TexCoord);
	}

	float ambientStrength = 0.65;
	vec3 ambient = ambientStrength * u_LightColor;

	float diff = max(dot(normalize(fragmentNormal), normalize(u_LightPosition - fragmentWorldPosition)), 0.0);
	vec3 diffuse = vec3(diff * u_LightColor);

	color = vec4((ambient + diffuse), 1.0) * objColor;
};
