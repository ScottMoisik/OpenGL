#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in mat4 rotModel;


void main() {
	gl_Position = instanceModel * rotModel * vec4(position, 1.0);
	mat3 normalMatrix = mat3(transpose(inverse(instanceModel * rotModel)));
}

#shader geometry
#version 330 core

uniform mat4 u_Proj;
uniform mat4 u_View;

layout(triangles) in;
layout(triangle_strip, max_vertices = 5) out;

const float PI = 3.14159265358979323846f;  // pi;
const float PI6 = PI / 6.0f;
const float PI3 = PI / 3.0f;
const float PI23 = 3.0f * (PI / 2.0f);
const float arrowHeadTip = 1.0f;
const float arrowHeadBase = 0.8f;
const float headScale = 0.5;
const float shaftScale = 0.5;


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


	/*
	

	//Arrow head
	
		arrowHeadTip,	0.0f,						0.0f,						//arrow tip
		arrowHeadBase,	headScale*sin(PI6),			headScale*cos(PI6),			//arrow head base, upper right
		arrowHeadBase,	headScale*sin(PI6 + PI3),	headScale*cos(PI6 + PI3),	//arrow head base, upper left
		arrowHeadBase,	headScale*sin(PI23),		headScale*cos(PI23)			//arrow head base, bottom
		});

	//Arrow shaft lambda
	auto buildArrowLambda = [&arrow, &arrowHeadBase, &shaftScale](float theta) {
		arrow->m_Positions.insert(arrow->m_Positions.begin(), {
			arrowHeadBase,	shaftScale*sin(theta),		shaftScale*cos(theta),		//arrow shaft tip
			0.0f,			shaftScale*sin(theta),		shaftScale*cos(theta),		//arrow shaft base
			});
	};

	buildArrowLambda(PI6);
	buildArrowLambda(PI6 + PI3);
	buildArrowLambda(PI23);

	arrow->m_VertexIndices.insert(arrow->m_VertexIndices.end(), {
		//Begin arrow head
		0, 1, 2,	0, 2, 3,	0, 3, 1,
		//Begin arrow shaft
		4, 5, 6,	6, 7, 4,
		7, 6, 9,	9, 8, 7,
		8, 9, 10,	10, 11, 8 });
		*/
}

#shader fragment
#version 330 core
out vec4 FragColor;

void main() {
	FragColor = vec4(1.0, 0.0, 0.2, 1.0);
}
