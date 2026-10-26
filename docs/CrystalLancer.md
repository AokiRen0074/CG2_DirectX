# 追加したモデル

参考画像に合わせた結晶プレイヤー本体と、水色の結晶を槍先にした槍です。

ゲーム画面のプレイヤー本体・槍をこのモデルへ差し替えています。地面と背景にはAL4のモデルを使用しています。コアと槍先は面の色と形に沿って発光し、ブルームで周囲へ柔らかく光が広がります。装甲には軽い浮遊を付けています。操作・当たり判定・赤青の軌跡の形は既存のままです。

## プロジェクト内

- `Resources/AL4/Player/CrystalLancer.blend`：プレイヤー本体と槍を編集するBlenderデータ。
- `Resources/AL4/Player/core.obj`：コア。
- `Resources/AL4/Player/armor_0.obj`～`armor_3.obj`：浮遊装甲。
- `Resources/AL4/Player/spear_shaft.obj`：槍の柄・継ぎ目。
- `Resources/AL4/Player/spear_blade.obj`：結晶の槍先。
- 各OBJと同名のMTL：面の色。
- `Resources/AL4/Runtime/`：AL4にあった地面3種類と背景モデルを、ゲーム用の寸法とマテリアル単位のメッシュへ変換した追加リソース。元のファイルも維持しています。

## 同梱ファイル

このフォルダーの`Runtime/Player`がプロジェクトの`Resources/AL4/Player`に、`Runtime/Environment`がプロジェクトの`Resources/AL4/Runtime`に対応します。

`CrystalLancer.blend`はBlender 5.1で編集できます。`CrystalLancer.glb`はマテリアルを含む交換用ファイルです。確認画像はBlenderでの完成イメージです。

ゲーム用OBJはYが上、Xの正方向が槍先方向になるよう、現在のローダーのX反転を補正してあります。マテリアルごとにメッシュを分け、三角形化済みです。外部テクスチャは不要です。

コア・装甲・槍は別パーツです。Blenderデータにはボーンやアニメーションは付いていません。ゲーム内の浮遊や照準に応じた槍の向きはゲーム側で制御しています。

## 確認

OBJの頂点・法線・面の向き・マテリアル分割、AL4モデルの面数保持、地面上面の基準位置を検証済みです。モデル差し替え後のビルドとゲーム内の待機・突進時の描画を確認しています。
