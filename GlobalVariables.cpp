#define NOMINMAX 
#include <Windows.h>

#include "GlobalValiables.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include <cassert>
#include <fstream>


GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

// 指定のオブジェクトがなければ追加する
void GlobalVariables::CreateGroup(const std::string& groupName) { datas_[groupName]; }

// 項目のセット(int)
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, int32_t value) {

	// グループの参照を取得
	Group& group = datas_[groupName];

	group[key] = value;
}

// 項目のセット(float)
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, float value) {

	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	group[key] = value;
}

// 項目のセット(Vetor3)
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const  Vector3 value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	group[key] = value;
}

#ifdef USE_IMGUI
// 更新処理
void GlobalVariables::Update() {
	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	if (!ImGui::BeginMenuBar())
		return;

	// 各グループについて
	for (std::map<std::string, Group>::iterator itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup) {

		// グループ名を取得
		const std::string& groupName = itGroup->first;
		// グループの参照を取得
		Group& group = itGroup->second;

		if (!ImGui::BeginMenu(groupName.c_str()))
			continue;

		// 各項目について
		for (std::map<std::string, Item>::iterator itItem = group.begin(); itItem != group.end(); ++itItem) {

			// 項目名を取得
			const std::string& itemName = itItem->first;

			// 項目の参照を取得
			Item& item = itItem->second;

			// int32_t型の値を保持していれば 
			if (std::holds_alternative<int32_t>(item)) {

				int32_t* ptr = std::get_if<int32_t>(&item);
				ImGui::SliderInt(itemName.c_str(), ptr, 0, 100);

			}
			else if (std::holds_alternative<float>(item)) {

				float* ptr = std::get_if<float>(&item);
				ImGui::SliderFloat3(itemName.c_str(), ptr, 0.0f, 100.0f);

			}
			else if (std::holds_alternative< Vector3>(item)) {

				 Vector3* ptr = std::get_if< Vector3>(&item);
				ImGui::SliderFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), -10.0f, 10.0f);
			}
		}

		ImGui::Text("\n");

		if (ImGui::Button("Save")) {

			SaveFile(groupName);

			std::string message = std::format("{}.json saved.", groupName);
			MessageBoxA(nullptr, message.c_str(), "GroubalVariavles", 0);
		}

		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
	ImGui::End();
}
#endif

void GlobalVariables::SaveFile(const std::string& groupName) {
	// グループを探索
	std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);

	// 未登録チェック
	assert(itGroup != datas_.end());

	json root;

	root = json::object();

	// jsonオブジェクト登録
	root[groupName] = json::object();

	// 各項目について
	for (std::map<std::string, Item>::iterator itItem = itGroup->second.begin(); itItem != itGroup->second.end(); ++itItem) {

		// 項目を取得
		const std::string& itemName = itItem->first;

		// 項目の参照を取得
		Item& item = itItem->second;

		// int32_t型の値を保持していれば
		if (std::holds_alternative<int32_t>(item)) {

			// int32_t型の値を登録
			root[groupName][itemName] = std::get<int32_t>(item);
		}
		else if (std::holds_alternative<float>(item)) {
			// float型の値を登録
			root[groupName][itemName] = std::get<float>(item);
		}
		else if (std::holds_alternative< Vector3>(item)) {

			// Vector3型のjson配列
			 Vector3 value = std::get< Vector3>(item);
			root[groupName][itemName] = json::array({ value.x, value.y, value.z });
		}
	}

	// ディレクトリがなければ作成する
	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(dir)) {
		std::filesystem::create_directory(dir);
	}

	// 書き込むJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + groupName + ".json";

	// 書き込むようファイルストリーム
	std::ofstream ofs;

	// 第るを書き込みように開く
	ofs.open(filePath);

	// ファイルオープン失敗？
	if (ofs.fail()) {

		std::string message = "Faild open data file for write. ";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}

	// ファイルにjson文字列を書き込む(インデント幅4)
	ofs << std::setw(4) << root << std::endl;

	// ファイルを閉じる
	ofs.close();
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	// 読み込むJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + groupName + ".json";
	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// 　ファイル読み込みように開く
	ifs.open(filePath);

	// ファイルオープン失敗?
	if (ifs.fail()) {
		std::string message = "Failed open data file for read. " + filePath;
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}

	json root;

	// json文字列からjsonデータ構造に天界
	ifs >> root;
	// ファイルを閉じる
	ifs.close();

	// 　グループを検索
	json::iterator itGroup = root.find(groupName);

	// 未登録チェック
	assert(itGroup != root.end());

	// 各アイテムについて
	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {

		// アイテム名を取得
		const std::string& itemName = itItem.key();

		// int32_t型の値を保持していれば
		if (itItem->is_number_integer()) {
			// int型の値を登録
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		}
		else if (itItem->is_number_float()) {
			// float 型の値を登録
			double value = itItem->get<double>();
			SetValue(groupName, itemName, static_cast<float>(value));
		}
		else if (itItem->is_array() && itItem->size() == 3) {
			// float型のjson配列
			 Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
			SetValue(groupName, itemName, value);
		}
	}
}

void GlobalVariables::LoadFiles() {
	// 保存先ディレクトリのパスをローカル変数で宣言
	std::filesystem::path dir(kDirectoryPath);

	// ディレクトリがなければスキップする
	if (!std::filesystem::exists(dir)) {
		return;
	}

	std::filesystem::directory_iterator dir_it(dir);
	for (const std::filesystem::directory_entry& entry : dir_it) {
		// ファイルパスを取得
		const std::filesystem::path& filePath = entry.path();

		// ファイル拡張子を取得
		std::string extension = filePath.extension().string();
		// .jsonファイル以外はスキップ
		if (extension.compare(".json") != 0) {
			continue;
		}

		// ファイル読み込み
		LoadFile(filePath.stem().string());
	}
}



// 項目の追加(int)
void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, int32_t value) {
	Group& group = datas_[groupName];

	// 項目が未登録なら
	if (group.find(key) == group.end()) {
		// SetValueを呼び出して値をセットする
		SetValue(groupName, key, value);
	}
}

// 項目の追加(float)
void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, float value) {
	Group& group = datas_[groupName];

	// 項目が未登録なら
	if (group.find(key) == group.end()) {
		SetValue(groupName, key, value);
	}
}

// 項目の追加(Vector3)
void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const  Vector3& value) {
	Group& group = datas_[groupName];

	// 項目が未登録なら
	if (group.find(key) == group.end()) {
		SetValue(groupName, key, value);
	}
}



// int型の値を取得
int32_t GlobalVariables::GetIntValue(const std::string& groupName, const std::string& key) const {
	assert(datas_.find(groupName) != datas_.end());
	// グループの参照を取得
	const Group& group = datas_.at(groupName);

	assert(group.find(key) != group.end());

	return std::get<int32_t>(group.at(key));
}

// float型の値を取得
float GlobalVariables::GetFloatValue(const std::string& groupName, const std::string& key) const {

	assert(datas_.find(groupName) != datas_.end());
	// グループの参照を取得
	const Group& group = datas_.at(groupName);


	assert(group.find(key) != group.end());


	return std::get<float>(group.at(key));
}

// Vector3型の値を取得
 Vector3 GlobalVariables::GetVector3Value(const std::string& groupName, const std::string& key) const {

	assert(datas_.find(groupName) != datas_.end());
	// グループの参照を取得
	const Group& group = datas_.at(groupName);


	assert(group.find(key) != group.end());

	return std::get< Vector3>(group.at(key));
}