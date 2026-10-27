# プレイヤーの軌跡・粒子エディター

Visual Studioで **Debug / x64** を起動し、ゲーム画面で **F2** を押します。
「Player trail / particles」を開くと、ゲームを止めたままダッシュと命中の光を繰り返し確認できます。

| 項目 | 調整するもの |
| --- | --- |
| Trail width | 赤青の帯の太さ |
| Ribbon spread | 帯が左右へ広がる幅 |
| Ribbon twists | 帯が交差する回数 |
| Trail lifetime (frames) | 軌跡が残る時間。60で約1秒 |
| Emission | 軌跡の発光の強さ。ブルームのにじみにも影響 |
| Blue / Red ribbon | 赤青それぞれの色。小さな光粒にも反映 |
| Particle size / speed / lifetime | ダッシュ・命中の光粒の大きさ、飛ぶ速さ、残る時間 |
| Dash particle interval | ダッシュ時に光粒を出す間隔。小さいほど多い |
| Impact particle count | 命中時の光粒の数 |

- **Replay preview**：プレビューを最初から再生。
- **Reset effects only**：エフェクトの設定だけ初期値へ戻す。
- **Save**：現在の設定を保存。
- **Reload**：保存した設定を読み直す。
- **F2**：エディターを閉じて実際に操作する。

保存先は起動したゲームの作業フォルダー内の `Settings/spear-editor.json`。通常のDebug起動ならリポジトリ内の `artifacts/bin/x64/Debug/Settings/spear-editor.json` です。設定は起動構成ごとに保存されます。前回保存分は `.bak` に残します。以前の設定ファイルもそのまま読み込めます。

プレビューは専用の仮の移動経路を使うため、プレイヤーの位置・HPや敵には影響しません。調整対象はプレイヤーの赤青の帯と光粒で、ボスの弾や着弾演出は従来の設定です。

軌跡はダッシュ中の座標を記録し、その点をつないだ帯を交差させて描画します。粒子は位置・速度・寿命を持つ小さな光として更新し、HDRの発光レイヤーとブルームで仕上げています。Blenderモデルに軌跡を焼き込んでいるわけではありません。
