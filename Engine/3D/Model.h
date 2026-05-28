#pragma once
#include <vector>
#include "Object3d.h"
#include "Sprite.h"
#include <string>
#include <map>



/*------------------------------------
1つのマテリアルのデータ
-----------------------------------*/
struct MaterialData {
	std::string name;// マテリアルの名前
	std::string textureFilePath;
	Vector4 diffuseColor = { 1.0f,1.0f,1.0f,1.0f};// 色情報

	// UVTransform情報
	Vector3 textureScale = { 1.0f,1.0f,1.0f }; 
	Vector3 textureOffset = { 0.0f,0.0f,0.0f };
};

/*-----------------------------
1つのメッシュのデータ
^------------------------------------*/
struct MeshData {
	std::string name; // メッシュの名前
	std::vector<VertexData> vertices;// 頂点データ
	std::string useMaterialName;// このパーツが使うマテリアルの名前
};


/*-----------------------------------
モデル全体のデータ
-------------------------------------*/
struct ModelData {
	std::vector<MeshData> meshes;
	std::map<std::string, MaterialData> materials;// 複数のマテリアル
};


ModelData LoadObjectFile(const std::string& directoryPath, const std::string& filename);

std::map<std::string, MaterialData> LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);