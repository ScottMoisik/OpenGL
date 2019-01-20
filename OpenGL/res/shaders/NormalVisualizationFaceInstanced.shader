#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;

uniform mat4 u_Proj;
uniform mat4 u_View;

void main() {
	gl_Position = u_Proj * u_View * instanceModel * rotModel * vec4(position, 1.0);
	mat3 normalMatrix = mat3(transpose(inverse(instanceModel * rotModel)));
}

#shader geometry
#version 330 core


layout(triangles) in;
layout(line_strip, max_vertices = 9) out;

void main() {
	vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
	vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
	vec4 norm = vec4(normalize(cross(a, b))* 2.75, 0.0) ;

	vec4 center = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position);
	gl_Position = center;
	EmitVertex();
	vec4 arrowTip = center + norm;
	gl_Position = arrowTip;
	EmitVertex();

	vec4 arrowHead = center + (norm * 1.8);
	vec4 normOrth1 = vec4(normalize(cross(a, vec3(norm))) * 0.8, 0.0);
	vec4 normOrth2 = vec4(normalize(cross(vec3(normOrth1), vec3(norm))) * 0.8, 0.0);

	gl_Position = center + arrowHead + normOrth1;
	EmitVertex(); //3 ('upper' head)

	gl_Position = arrowTip;
	EmitVertex(); //4 (back to arrow tip)

	gl_Position = center + arrowHead - normOrth1;
	EmitVertex(); //5 ('lower' head)

	gl_Position = arrowTip;
	EmitVertex(); //6 (back to arrow tip)

	gl_Position = center + arrowHead + normOrth2;
	EmitVertex(); //7 ('upper' head)

	gl_Position = arrowTip;
	EmitVertex(); //8 (back to arrow tip)

	gl_Position = center + arrowHead - normOrth2;
	EmitVertex(); //9 ('lower' head)

	EndPrimitive();

}

#shader fragment
#version 330 core
out vec4 FragColor;

void main() {
	FragColor = vec4(1.0, 0.0, 0.2, 1.0);
}
