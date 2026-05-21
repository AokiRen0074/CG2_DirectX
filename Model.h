#pragma once
#include <vector>
#include "Object3d.h"
#include "Sprite.h"

struct ModelData {
	std::vector<VertexData> vertices;
};

class Model {
public:
	void LoadObjectFile(const std::string& directoryPath, const std::string& filename);

private:


};