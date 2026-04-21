#pragma once
#include <string>
#include <fstream>

class Logger {
public:
    // 初期化 ログファイルの作成
    static void Initialize();
    // 終了処理 ファイルのクローズ
    static void Finalize();

    // ログ出力
    static void Log(const std::string& message);

    // 文字列変換
    static std::wstring ConvertString(const std::string& str);
    static std::string ConvertString(const std::wstring& str);

private:
    static std::ofstream* logStream_; // ログファイルへのストリーム
};