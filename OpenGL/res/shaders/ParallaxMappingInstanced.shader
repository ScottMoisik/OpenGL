#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitanget;


uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_LightSpaceMatrix;

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
	vec4 posHomo = vec4(position, 1.0);
	vs_out.FragPos = vec3(u_Model * posHomo); //instanceModel * rotModel * posHomo);
	vs_out.Normal = transpose(inverse(mat3(u_Model)))* normal;//instanceModel * rotModel))) * normal;
	vs_out.TexCoords = texCoord;
	vs_out.FragPosLightSpace = u_LightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
	gl_Position = u_Proj * u_View * u_Model * posHomo;//instanceModel * rotModel * posHomo;

	mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
	vec3 T = normalize(normalMatrix * tangent);
	vec3 N = normalize(normalMatrix * normal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));
	vs_out.TangentLightPos = TBN * u_LightPos;
	vs_out.TangentViewPos = TBN * u_ViewPos;
	vs_out.TangentFragPos = TBN * vs_out.FragPos;


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

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_DepthMap;

uniform float heightScale;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
	
	// number of depth layers
	const float minLayers = 8;
	const float maxLayers = 32;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
	// calculate the size of each layer
	float layerDepth = 1.0 / numLayers;
	// depth of current layer
	float currentLayerDepth = 0.0;
	// the amount to shift the texture coordinates per layer (from vector P)
	vec2 P = viewDir.xy / viewDir.z * heightScale;
	vec2 deltaTexCoords = P / numLayers;

	// get initial values
	vec2  currentTexCoords = texCoords;
	float currentDepthMapValue = texture(u_DepthMap, currentTexCoords).r;

	while (currentLayerDepth < currentDepthMapValue) {
		// shift texture coordinates along direction of P
		currentTexCoords -= deltaTexCoords;
		// get depthmap value at current texture coordinates
		currentDepthMapValue = texture(u_DepthMap, currentTexCoords).r;
		// get depth of next layer
		currentLayerDepth += layerDepth;
	}

	// get texture coordinates before collision (reverse operations)
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(u_DepthMap, prevTexCoords).r - currentLayerDepth + layerDepth;

	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	
	return finalTexCoords;
}


void main() {
	// offset texture coordinates with Parallax Mapping
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec2 texCoords = fs_in.TexCoords;

	texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
	if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
		discard;

	// obtain normal from normal map
	vec3 normal = texture(u_NormalMap, texCoords).rgb;
	normal = normalize(normal * 2.0 - 1.0);

	// get diffuse color
	vec3 color = texture(u_DiffuseMap, texCoords).rgb;
	
	// Ambient 
	vec3 ambient = 0.65 * color;

	// Diffuse 
	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * color;

	// Specular 
	//vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
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

	//FragColor = objColor;
	//FragColor = vec4(ambient, 1.0f) + ((1.0 - shadow) * objColor);
	//FragColor = vec4(ambient + (1.0 - shadow) * vec3(1.0f, 0.0f, 0.0f), 1.0f);//vec4(color, 1.0f);//
	FragColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)), 1.0f);

};

