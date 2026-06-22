#pragma once
#include <variant>
#include <string.h>
#include<map>
#include <externals/nlohmann/json.hpp>
#include "Vector3.h"

class GlobalVariables {
public:

	static GlobalVariables* GetInstance();
	void CreateGroup(const std::string& groupName);

	// 値のセット(int)
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);

	// 値のセット(float)
	void SetValue(const std::string& groupName, const std::string& key, float value);

	// 値のセット(Vector3)
	void SetValue(const std::string& groupName, const std::string& key, const Vector3 value);

	// 毎フレーム更新処理
	void Update();

	// ファイルに書き出し
	void SaveFile(const std::string& groupName);



	// ディレクトリの全ファイル読み込み
	void LoadFiles();

	// ファイルから読み込む
	void LoadFile(const std::string& groupName);

	// 項目の追加(int)
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);

	// 項目の追加(float
	void AddItem(const std::string& groupName, const std::string& key, float value);

	// 項目の追加(Vector3)
	void AddItem(const std::string& groupName, const std::string& key, const Vector3& value);

	// 値の取得(ゲッター)
	int32_t GetIntValue(const std::string& groupName, const std::string& key) const;
	float GetFloatValue(const std::string& groupName, const std::string& key) const;
	Vector3 GetVector3Value(const std::string& groupName, const std::string& key) const;


private:

	// コンストラクタ
	GlobalVariables() = default;

	// デストラクタ
	~GlobalVariables() = default;

	// コピーコンストラクタ
	GlobalVariables(const GlobalVariables&) = delete;

	//　コピー代入演算子
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	// 項目の値
	using Item = std::variant < int32_t, float,Vector3 >;


	using Group = std::map<std::string, Item>;

	// 全データ
	std::map<std::string, Group> datas_;

	using json = nlohmann::json;

	// グローバル変数の保存先ファイルパス 
	const std::string kDirectoryPath = "Resources/GlobalVariables/";


};
