#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;
layout(location = 11) in vec3 tangent;
layout(location = 12) in vec3 bitanget;

uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_LightSpaceMatrix;
uniform int u_UseNormalMapping;

out VS_OUT{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} vs_out;

/*
out vec2 v_TexCoord;
out vec3 f_Normal;
out vec3 f_WorldPosition;
*/
//flat out int InstanceID;

void main() {
	mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
	vec3 T = normalize(normalMatrix * tangent);
	vec3 N = normalize(normalMatrix * normal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));
	vs_out.TangentLightPos = TBN * u_LightPos;
	vs_out.TangentViewPos = TBN * u_ViewPos;
	vs_out.TangentFragPos = TBN * vs_out.FragPos;

	vec4 posHomo = vec4(position, 1.0);
	vs_out.FragPos = vec3(instanceModel * rotModel * posHomo);
	vs_out.Normal = transpose(inverse(mat3(instanceModel * rotModel))) * normal;
	vs_out.TexCoords = texCoord;
	vs_out.FragPosLightSpace = u_LightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
	gl_Position = u_Proj * u_View * instanceModel * rotModel * posHomo;

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
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;


out vec4 FragColor;

uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;

uniform sampler2D u_ShadowMap;
uniform sampler2D u_Texture;
uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;

void main() {
	
	// obtain normal from normal map in range [0,1]
	vec3 normal = texture(u_NormalMap, fs_in.TexCoords).rgb;
	// transform normal vector to range [-1,1]
	normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space

	// get diffuse color
	vec3 color = texture(u_DiffuseMap, fs_in.TexCoords).rgb;

	// Ambient 
	vec3 ambient = 0.65 * color;
	
	// Diffuse 
	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * color;
	
	// Specular 
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float specularStrength = 0.5;
	float spec = 0.0;
	bool blinnFlag = true;
	if (blinnFlag) {
		vec3 halfwayDir = normalize(lightDir + viewDir);
		spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
	} else {
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

	}
	vec3 specular = vec3(0.2) * spec;

	// calculate shadow 
	//float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
	// perform perspective divide
	vec3 projCoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(u_ShadowMap, projCoords.xy).x;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float bias = 0.005;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;
	if (projCoords.z > 1.0)
		shadow = 0.0;

	//FragColor = vec4(ambient, 1.0f) + ((1.0 - shadow) * objColor);
	//FragColor = objColor;
	FragColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)), 1.0f);
	
};

