# 汎用エンジン機能の取り込み

取り込み元: `AL4個人制作` (`b2fe286`)。取り込み前の master: `2acccaa`。

## 取り込んだ機能

- `Engine/Scene`: シーンの所有権とライフサイクル、更新終了後の安全なシーン遷移。
- `Engine/Audio`: Media Foundation による音声読み込み、ループ・停止・一時停止・音量・ピッチ、再利用されない VoiceId、再生中の音声データの寿命管理。
- `Engine/2D`: スプライトの拡縮・回転・画像切り取り、独立した描画パイプライン、テクスチャサイズ取得、UTF-8 パス。
- `Engine/3D`: モデルと GPU 頂点データの共有、モデル切り替え、個別の座標とマテリアル、テクスチャなし描画、ブレンド、ライティング、パーティクル、カメラシェイク、デバッグカメラ改善。
- `Engine/PostProcess`: HDR ブルーム、発光レイヤーとぼかし、画面の放射状ブラー。シーンへの適用は呼び出し側で選択する。
- `Engine/Collision`: 球の衝突判定とビットマスクによるフィルタリング。プレイヤー・敵の固定属性は取り込まない。
- `Engine/Math`: ベクトル演算、Slerp、法線変換、Catmull-Rom 補間、遅延コールバック、明滅。
- 基盤: ログの所有権管理、フェンスイベント解放、描画先の復帰、ビルド成果物と実行時ファイルの配置。

## 互換性と調整

`Sprite::Create` と `Object3d::Create` は master と同じ生ポインタを返す。
新しいコードでは `CreateOwned` を使うと `std::unique_ptr` で管理できる。
既存 Player のヘッダー依存を明示し、アプリ終了時にモデルキャッシュを解放する。
Application のゲーム内容は移植していない。

フレーム数 0 の TimedCall が符号なし整数の減算で待ち続けないよう修正。
Collider に仮想デストラクターを追加。
Release は DirectXTex とランタイム設定を一致させ、インクルード検索パスも修正。

## 除外したもの

取り込み元の Application 全体、キャラクター・敵・弾・レール・ステージ・エディター・UI・ゲーム進行と素材。
Engine/Neon のゲーム向けクラスと旧バージョン、ゲーム専用メッシュと演出を含む EffectRenderer、重複する旧 BodyModel。
ゲームに依存するテスト・ドキュメント・外部ライブラリの変更。

## 検証

Visual Studio の x64 Native Tools 環境で以下を実行する。

```bat
msbuild CG2_DirectX.sln /m /p:Configuration=Debug /p:Platform=x64
msbuild CG2_DirectX.sln /m /p:Configuration=Release /p:Platform=x64
tests\run-scene-manager-tests.cmd
tests\run-audio-voice-tests.cmd
tests\run-engine-utility-tests.cmd
tests\run-shader-compile-tests.cmd
```

音声テストは master に既存の Alarm01.wav を使用する。
シェーダーテストは 11 個のエントリーポイントを DXC で検証する。
画面上での描画の目視確認は別途必要。

## 起動処理とファイル構成の整理

- `Application/main.cpp`: 実行ファイルの場所を作業ディレクトリに設定し、アプリを起動する。
- `Application/GameApplication.h/.cpp`: 初期化・メインループ・終了処理。ゲーム専用のコマンドや演出処理は含めない。
- `Application/GameScene.h/.cpp`: master に既存のサンプルシーン。IScene を実装し SceneManager から更新・描画する。
- `Application/Character/Player.h/.cpp`: master に既存の Player をルートから移動。
- `Engine/3D/ViewProjection.h/.cpp`, `WorldTransform.h/.cpp`: 3D 基盤をルートから移動。
- Visual Studio のフィルターを実際のフォルダー階層に合わせ、テスト・ビルド設定・説明資料もプロジェクトに登録。

終了時はシーン、ImGui、共有リソース、デバイスの順に解放する。
ViewProjection の既定の遠クリップ面が 0 だった不具合は取り込み元と同じ 1000 に修正。
