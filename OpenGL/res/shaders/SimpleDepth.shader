#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;

uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;

void main() {
	//gl_Position = u_LightSpaceMatrix * instanceModel * rotModel * vec4(position, 1.0);
	gl_Position = u_LightSpaceMatrix * instanceModel * rotModel * vec4(position, 1.0);
}

#shader fragment
#version 330 core

void main() {
	//gl_FragDepth = gl_FragCoord.z;
}