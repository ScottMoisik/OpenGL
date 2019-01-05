#pragma once

#include "Renderer.h"
class Mesh;

class Model {
public:
	/*  Functions   */
	Model(const char* path);
	~Model();

	void Draw(Shader shader);
private:
	/*  Model Data  */
	//std::vector<Mesh> m_Meshes;
	std::string m_Directory;
	
	/*  Functions   */
	void loadModel(const char* path);
};

