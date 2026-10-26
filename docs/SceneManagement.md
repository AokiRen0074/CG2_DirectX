# シーン管理と所有権

Application/main.cppはGameApplication::Run()を呼ぶだけの7行です。
元のmain.cppのコメント、起動オプション、開発用の確認処理はGameApplication.cppに移しています。

## Stateパターン

- ISceneが入場・退場・更新・描画の共通インターフェースです。
- SceneManagerが現在と予約中のシーンをunique_ptrで所有します。
- PlayingSceneStateは既存のゲームを更新し、結果の入力が可能になったらResultSceneStateへ切り替えます。
- ResultSceneStateは結果表示とリプレイ入力を更新し、再開時にPlayingSceneStateへ戻ります。

ゲーム本体のGameSceneは二つの状態からshared_ptrで共有します。タイトル、クリア演出、暗転、モデル、音源は状態遷移のたびに作り直しません。
ChangeScene()は切り替えを予約するだけで、更新中のシーンをその場で破棄しません。

## 所有権

ゲームや描画オブジェクトの所有にはunique_ptr、共有モデルや音声の波形にはshared_ptr、COMの所有にはComPtrを使います。
GetDevice()などから借りたポインタは、所有権を受け取らないため生ポインタのままです。
XAudio2のボイスはCOMではないので、専用のデリーターを付けたunique_ptrで管理します。

## 確認

Visual Studioのx64 Native Tools Command Promptでtests\run-scene-manager-tests.cmdを実行すると、遷移と解放のテストを行えます。
--spear-loop-smoke-testは実際のゲームを進め、結果状態とリプレイ後のプレイ状態も確認します。