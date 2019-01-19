#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;

out VS_OUT{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} vs_out;

uniform mat4 u_Proj;
uniform mat4 u_View;

void main() {
	vec4 posHomo = vec4(position, 1.0);
	vs_out.FragPos = vec3(instanceModel * rotModel * posHomo);
	vs_out.Normal = transpose(inverse(mat3(instanceModel * rotModel))) * normal;
	gl_Position = u_Proj * u_View * instanceModel * rotModel * vec4(position, 1.0);
	vs_out.TexCoords = texCoord;
};


#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in VS_OUT{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs_in;

uniform vec4 u_Color;
uniform sampler2D u_Texture;
uniform vec3 u_LightColor;
uniform vec3 u_LightPosition;

void main() {
	vec4 texColor = texture(u_Texture, fs_in.TexCoords);
	// Ambient 
	vec4 ambient = vec4(0.65 * u_LightColor, 1.0);

	vec3 norm = normalize(fs_in.Normal);
	vec3 lightDir = normalize(u_LightPosition - fs_in.FragPos);
	float diff = abs(dot(norm, lightDir));
	color = ambient + vec4(diff * vec3(texColor), 1.0); //vec4(1.0f, 0.0f, 0.0f, 1.0f); //
};
