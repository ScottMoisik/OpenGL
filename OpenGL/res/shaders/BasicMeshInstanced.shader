#shader vertex
#version 330 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

out vec2 v_TexCoord;
out vec3 f_Normal;
out vec3 f_WorldPosition;
//flat out int InstanceID;

void main() {
	vec4 posHomo = vec4(position, 1.0);
	f_Normal = mat3(transpose(inverse(instanceModel * rotModel)))*normal;
	f_WorldPosition = vec3(instanceModel * rotModel * posHomo);
	gl_Position = u_Proj * u_View * instanceModel * rotModel * posHomo;
	v_TexCoord = texCoord;
	//InstanceID = gl_InstanceID;
};


#shader fragment
#version 330 core

in vec2 v_TexCoord;

in vec3 f_Normal;
in vec3 f_WorldPosition;

//layout(location = 1) in vec3 normal;
//layout(location = 0) out vec4 color;
out vec4 color;

uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;
uniform vec3 u_LightPosition;

uniform vec4 u_ObjectColor;

uniform int u_UseTexturing;

uniform sampler2D u_Texture;


void main() {
	
	vec4 objColor = u_ObjectColor;

	if (bool(u_UseTexturing)) {
		objColor = texture(u_Texture, v_TexCoord);
	}
		
	float ambientStrength = 0.5;
	vec3 ambient = ambientStrength * u_LightColor;
	
	vec3 norm = normalize(f_Normal);
	vec3 lightDir = normalize(u_LightPosition - f_WorldPosition);
	vec3 viewDir = normalize(u_ViewPos - f_WorldPosition);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = vec3(diff * u_LightColor);
	
	float specularStrength = 0.5;
	float spec = 0.0;
	bool blinnFlag = true;
	if (blinnFlag) {
		vec3 reflectDir = reflect(lightDir, normalize(f_Normal));
		spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
	} else {
		vec3 reflectDir = reflect(-lightDir, normalize(f_Normal));
		spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

	}
	vec3 specular = specularStrength * spec * u_LightColor;


	color = vec4((ambient + diffuse + specular) * objColor.xyz, objColor.w);
};
