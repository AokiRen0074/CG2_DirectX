#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

// ==========================================
// MTLファイルの読み込み関数
// ==========================================
std::map<std::string, MaterialData> LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	std::map<std::string, MaterialData> materialDataMap;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open()); // 開けなかったら止める

	MaterialData* currentMaterial = nullptr;

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// 新しいマテリアルの定義開始
		if (identifier == "newmtl") {
			std::string materialName;
			s >> materialName;

			// mapに新しいマテリアルの箱を作り、ポインタをそこに向ける
			materialDataMap[materialName] = MaterialData();
			currentMaterial = &materialDataMap[materialName];
			currentMaterial->name = materialName;
		}
		// マテリアル作成中のみ処理する
		else if (currentMaterial) {
			if (identifier == "map_Kd") {
				std::string token;
				while (s >> token) {
					if (token == "-s") {
						s >> currentMaterial->textureScale.x >> currentMaterial->textureScale.y >> currentMaterial->textureScale.z;
					}
					else if (token == "-o") {
						s >> currentMaterial->textureOffset.x >> currentMaterial->textureOffset.y >> currentMaterial->textureOffset.z;
					}
					else {
						currentMaterial->textureFilePath = directoryPath + "/" + token;
					}
				}
			}
			else if (identifier == "Kd") {
				Vector3 kd;
				s >> kd.x >> kd.y >> kd.z;
				currentMaterial->diffuseColor = { kd.x, kd.y, kd.z, 1.0f };
			}
		}
	}
	return materialDataMap;
}

// ==========================================
// OBJファイルの読み込み（複数パーツ対応）
// ==========================================
ModelData LoadObjectFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;

	MeshData* currentMesh = nullptr; 

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// マテリアルリストの読み込み
		if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			modelData.materials = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
		//メッシュの開始
		else if (identifier == "o") {
			std::string meshName;
			s >> meshName;

			// ModelDataの中に新しいパーツの箱を作る
			MeshData newMesh;
			newMesh.name = meshName;
			modelData.meshes.push_back(newMesh);

			// ポインタを最新のパーツに向ける
			currentMesh = &modelData.meshes.back();
		}
		// どのマテリアルを使うか
		else if (identifier == "usemtl") {
			std::string materialName;
			s >> materialName;
			if (currentMesh) {
				currentMesh->useMaterialName = materialName; // パーツに名前を記録
			}
		}

		else if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
	
			if (!currentMesh) {
				MeshData defaultMesh;
				defaultMesh.name = "default";
				modelData.meshes.push_back(defaultMesh);
				currentMesh = &modelData.meshes.back();
			}

			VertexData triangle[3];

			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3] = { 0, 0, 0 };
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					if (!index.empty()) {
						elementIndices[element] = std::stoi(index);
					}
				}

				// 頂点情報の取得
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = { 0.0f, 0.0f };
				if (elementIndices[1] > 0) texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = { 0.0f, 0.0f, 1.0f };
				if (elementIndices[2] > 0) normal = normals[elementIndices[2] - 1];

				// 左手系・UV変換
				position.x *= -1.0f;
				normal.x *= -1.0f;
				texcoord.y = 1.0f - texcoord.y;

				triangle[faceVertex] = { position, texcoord, normal };
			}

			currentMesh->vertices.push_back(triangle[2]);
			currentMesh->vertices.push_back(triangle[1]);
			currentMesh->vertices.push_back(triangle[0]);
		}
	}
	return modelData;
}