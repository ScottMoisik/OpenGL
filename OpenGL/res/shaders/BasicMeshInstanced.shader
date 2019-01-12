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
uniform mat4 u_LightSpaceMatrix;

out VS_OUT{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
} vs_out;

/*
out vec2 v_TexCoord;
out vec3 f_Normal;
out vec3 f_WorldPosition;
*/
//flat out int InstanceID;

void main() {

	vec4 posHomo = vec4(position, 1.0);
	vs_out.FragPos = vec3(instanceModel * rotModel * posHomo);
	vs_out.Normal = transpose(inverse(mat3(instanceModel * rotModel))) * normal;
	vs_out.TexCoords = texCoord;
	vs_out.FragPosLightSpace = u_LightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
	gl_Position = u_Proj * u_View * instanceModel * rotModel * posHomo;

	/*
	vec4 posHomo = vec4(position, 1.0);
	f_Normal = mat3(transpose(inverse(instanceModel * rotModel)))*normal;
	f_WorldPosition = vec3(instanceModel * rotModel * posHomo);
	gl_Position = u_Proj * u_View * instanceModel * rotModel * posHomo;
	*/
};


#shader fragment
#version 330 core

/*
in vec2 v_TexCoord;
in vec3 f_Normal;
in vec3 f_WorldPosition;
*/

in VS_OUT{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
} fs_in;


out vec4 FragColor;

uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;
uniform vec3 u_LightPosition;

uniform vec4 u_ObjectColor;

uniform int u_UseTexturing;

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;



float ShadowCalculation(vec4 fragPosLightSpace) {
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(u_ShadowMap, projCoords.xy).z;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

	return shadow;
}

void main() {
	
	vec4 objColor = u_ObjectColor;

	if (bool(u_UseTexturing)) {
		objColor = texture(u_Texture, fs_in.TexCoords);
	}
	
	/* Ambient */
	vec3 ambient = 0.15 * u_LightColor;
	
	/* Diffuse */
	vec3 norm = normalize(fs_in.Normal);
	vec3 lightDir = normalize(u_LightPosition - fs_in.FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * u_LightColor;
	
	/* Specular */
	vec3 viewDir = normalize(u_ViewPos - fs_in.FragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float specularStrength = 0.5;
	float spec = 0.0;
	bool blinnFlag = true;
	if (blinnFlag) {
		vec3 halfwayDir = normalize(lightDir + viewDir);
		spec = pow(max(dot(norm, halfwayDir), 0.0), 64.0);
	} else {
		vec3 reflectDir = reflect(-lightDir, norm);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

	}
	vec3 specular = specularStrength * spec * u_LightColor;

	/* calculate shadow */
	//float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
	// perform perspective divide
	vec3 projCoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(u_ShadowMap, projCoords.xy).g;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float shadow = currentDepth > closestDepth ? 1.0 : 0.5;


	FragColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)) * objColor.xyz, objColor.w);
	//shadow * objColor;//
};

