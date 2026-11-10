# CG2_DirectX

DirectX 12による自作エンジン。現在は授業ベースの描画基盤とサンプルPlayerを起動する構成。

- Application/main.cpp: 起動場所の準備とGameApplicationの実行
- Application/GameApplication: 初期化・メインループ・終了
- Application/GameScene: サンプルシーン
- Engine/PostProcess/Bloom: HDR発光と分離ガウシアンぼかし
- Engine/PostProcess/ScenePostProcess: 通常画面のポストエフェクト
- Engine/PostProcess/FullscreenPass: 共通の全画面描画

## ビルド

Visual StudioでCG2_DirectX.slnを開き、DebugまたはRelease / x64でビルドする。
実行時の素材とDLLはartifacts/bin/x64/構成へコピーされる。

## 資料

- [ポストプロセスの使い方と移行](docs/PostProcessing.md)
- [今回の問題点と修正内容](docs/EngineCleanup.md)
- [汎用エンジン取り込み時の構成](docs/GenericEngineIntegration.md)

## テスト

Visual Studioのx64 Native Tools環境でtests/run-*-tests.cmdを実行する。
run-post-process-tests.cmdはRelease版DirectXTex.libを使用するため、先にRelease構成をビルドする。
