#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

ModelData LoadObjectFile(const std::string& directoryPath, const std::string& filename) {
    ModelData modelData;
    std::vector<Vector4> positions; // 位置
    std::vector<Vector3> normals;   // 法線
    std::vector<Vector2> texcoords; // テクスチャ座標
    std::vector<Vector3> colors;
    std::string line;               // ファイルから読んだ1行を格納するもの

    //ファイルを開く
    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open()); // 開けなかったらここでプログラムを止める

    //  実際にファイルを読み、ModelDataを構築していく
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier; // 先頭の識別子を読む

        // identifierに応じた処理
        if (identifier == "v") {
            Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.w = 1.0f; // 同次座標系なので w は 1.0
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
            // 面は三角形
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefinition;
                s >> vertexDefinition;

                // 頂点の要素へのIndexを取得する
                std::istringstream v(vertexDefinition);
                uint32_t elementIndices[3];
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(v, index, '/'); // '/' 区切りでインデックスを読んでいく
                    if (!index.empty()) {
                        elementIndices[element] = std::stoi(index);
                    }
                }


                Vector4 position = positions[elementIndices[0] - 1];
                Vector2 texcoord = texcoords[elementIndices[1] - 1];
                Vector3 normal = normals[elementIndices[2] - 1];

                VertexData vertex = { position, texcoord, normal };
                modelData.vertices.push_back(vertex);
            }
        }
    }

    // ModelDataを返す
    return modelData;
}