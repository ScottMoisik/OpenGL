#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

out vec2 TexCoords;

void main() {
	TexCoords = texCoord;
	gl_Position = vec4(position, 1.0);
}

#shader fragment
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float u_NearPlane;
uniform float u_FarPlane;
uniform int u_OrthographicFlag;

// required when using a perspective projection matrix
float LinearizeDepth(float depth) {
	float z = depth * 2.0 - 1.0; // Back to NDC 
	return (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - z * (u_FarPlane - u_NearPlane));
}

void main() {
	float depthValue = texture(depthMap, TexCoords).r;
	if (bool(u_OrthographicFlag)) {
		FragColor = vec4(vec3(depthValue), 1.0); // orthographic
	} else {
		FragColor = vec4(vec3(LinearizeDepth(depthValue) / u_FarPlane), 1.0); // perspective

	}
	//FragColor = vec4(0.2, 0.4, 0.2, 1.0);
}