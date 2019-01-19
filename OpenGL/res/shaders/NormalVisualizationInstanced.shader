#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;

out VS_OUT{
	vec3 normal;
} vs_out;

uniform mat4 u_Proj;
uniform mat4 u_View;



void main() {
	gl_Position = u_Proj * u_View * instanceModel * rotModel * vec4(position, 1.0);
	mat3 normalMatrix = mat3(transpose(inverse(instanceModel * rotModel)));
	vs_out.normal = normalize(vec3(u_Proj * u_View * vec4(normalMatrix * normal, 0.0)));
}

#shader geometry
#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in VS_OUT{
	vec3 normal;
} gs_in[];

const float MAGNITUDE = 0.4;


void GenerateLine(int index) {
	gl_Position = gl_in[index].gl_Position;
	EmitVertex();
	gl_Position = gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE;
	EmitVertex();
	EndPrimitive();
}

void main() {
	GenerateLine(0); // first vertex normal
	GenerateLine(1); // second vertex normal
	GenerateLine(2); // third vertex normal
}

#shader fragment
#version 330 core
out vec4 FragColor;

void main() {
	FragColor = vec4(1.0, 1.0, 0.0, 1.0);
}
