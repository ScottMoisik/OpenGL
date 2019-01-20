#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 normal;
//layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;


void main() {
	gl_Position = instanceModel * rotModel * vec4(position, 1.0);
	mat3 normalMatrix = mat3(transpose(inverse(instanceModel * rotModel)));
};

#shader geometry
#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 16) out;

uniform mat4 u_ProjView;


void main() {
	float PI = 3.14159265358979323846f;  // pi;
	float PI6 = PI / 6.0f;
	float PI3 = PI / 3.0f;
	float PI23 = 3.0f * (PI / 2.0f);
	float arrowHeadBase = 0.8f;
	float headScale = 0.5;
	float shaftScale = 0.5;
	float oneThird = 1.0f / 3.0f;

	vec3 a = vec3(gl_in[1].gl_Position) - vec3(gl_in[0].gl_Position);
	vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[0].gl_Position);
	vec3 norm = normalize(cross(a, b));
	
	// Build rotation matrix to rotate arrow components to the orientation of the normal
	//https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d/897677
	vec3 arrow = vec3(1.0, 0.0, 0.0);
	vec3 cp = cross(arrow, norm);
	//float s = length(cp);
	float c = dot(arrow, norm);

	//Fill matrices in GLSL by column (column major ordering)
	mat3 skewSym = mat3(
		0.0, cp.z, -cp.y,
		-cp.z, 0.0, cp.x,
		cp.y, -cp.x, 0.0);
	mat3 R = mat3(1.0) + skewSym + ((skewSym*skewSym)*(1.0 / (1.0 + c)));
	vec4 normArrow = vec4(R * arrow, 0.0);

	vec4 center = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) * oneThird;
	
	//arrow head point 0 (base)
	vec4 arrowHeadBaseVec = u_ProjView * vec4(1.0, 0.0, 0.0, 0.0);//vec4(R * vec3(arrowHeadBase, headScale*sin(PI6), headScale*cos(PI6)), 0.0f);
	gl_Position = arrowHeadBaseVec;
	EmitVertex();

	//arrow head point 1 (tip)
	vec4 arrowHeadTip = u_ProjView * (center + normArrow);
	gl_Position = arrowHeadTip;
	EmitVertex();

	//arrow head point 2 (base)
	gl_Position = u_ProjView * vec4(1.0, 0.0, 0.0, 0.0);//vec4(R * vec3(arrowHeadBase, headScale*sin(PI6 + PI3), headScale*cos(PI6 + PI3)), 0.0f);
	EmitVertex();

	//arrow head point 3 (base)
	gl_Position = arrowHeadTip;//u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI23), headScale*cos(PI23)), 0.0f);
	EmitVertex();

	//arrow head point 4 (base)
	gl_Position = arrowHeadBaseVec;
	EmitVertex();

	//arrow head point 5 (tip)
	gl_Position = arrowHeadTip;
	EmitVertex();
	EndPrimitive();


	/*
	//arrow head shaft 0 (bottom)
	vec4 arrowShaftBottom = u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI6), headScale*cos(PI6)), 0.0f);
	gl_Position = arrowShaftBottom;
	EmitVertex();

	//arrow head shaft 1 (top)
	vec4 arrowShaftTop = u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI6), headScale*cos(PI6)), 0.0f);
	gl_Position = arrowShaftTop;
	EmitVertex();

	//arrow head shaft 2 (bottom)
	gl_Position = u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI6 + PI3), headScale*cos(PI6 + PI3)), 0.0f);
	EmitVertex();

	//arrow head shaft 3 (top)
	gl_Position = u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI6 + PI3), headScale*cos(PI6 + PI3)), 0.0f);
	EmitVertex();

	//arrow head shaft 4 (bottom)
	gl_Position = u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI23), headScale*cos(PI23)), 0.0f);
	EmitVertex();

	//arrow head shaft 5 (top)
	gl_Position = u_ProjView * vec4(R * vec3(arrowHeadBase, headScale*sin(PI23), headScale*cos(PI23)), 0.0f);
	EmitVertex();

	//arrow head shaft 7 (bottom)
	gl_Position = arrowShaftBottom;
	EmitVertex();

	//arrow head shaft 8 (top)
	gl_Position = arrowShaftTop;
	EmitVertex();

	EndPrimitive();
	*/
};

#shader fragment
#version 330 core

out vec4 FragColor;

void main() {
	FragColor = vec4(1.0, 0.0, 0.2, 1.0);
};
