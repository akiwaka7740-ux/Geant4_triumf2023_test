# triumf2023_test コード概要

## 目的

このディレクトリは、Geant4 を用いた検出器シミュレーションコードです。README によると、UROKO、HILE、Li-glass などの検出器を単体でテストするためのプログラムとして作られています。

現在の主な関心は、UROKO 検出器におけるシンチレーション光の発生、光学光子の輸送、PMT への到達数、収集効率を評価することです。

## 全体構成

- `sim.cc`
  - 実行ファイルのエントリーポイントです。
  - 乱数シードを毎回生成し、Geant4 の RunManager、PhysicsList、DetectorConstruction、ActionInitialization を登録します。
  - マルチスレッドビルド時は、CPU コア数から 4 を引いたスレッド数で実行します。
  - UI セッションと可視化マネージャを初期化します。

- `CMakeLists.txt`
  - Geant4 の `ui_all` と `vis_all` を要求します。
  - `src/**/*.cc` を再帰的に集め、`sim` 実行ファイルを作ります。
  - `macros/*.mac` をビルドディレクトリへコピーします。

- `include/`
  - 各クラスのヘッダファイルがあります。
  - 検出器構築、物理リスト、イベント処理、Run 処理、SensitiveDetector、材料、光学表面、各検出器の LogicalVolume が分かれています。

- `src/`
  - Geant4 の実装コード本体です。
  - `Material/`、`Surface/`、`LogVol/` に、材料・光学面・検出器形状の定義が整理されています。

- `macros/`
  - `run.mac`: `/run/initialize` 後、1000000 event を実行します。
  - `vis.mac`: OpenGL 可視化、軌跡表示、軸表示などを設定します。

- `root/`
  - `RunAction` から ROOT 出力ファイルが保存される想定のディレクトリです。

## 実行時の流れ

1. `sim.cc` が乱数シードを設定します。
2. `PhysicsList` が物理過程を登録します。
3. `DetectorConstruction` が World と各検出器を構築します。
4. `ActionInitialization` が以下の UserAction を登録します。
   - `PrimaryGenerator`
   - `RunAction`
   - `EventAction`
   - `SteppingAction`
5. イベントごとに粒子が発生し、検出器内でのエネルギー付与、光学光子発生、PMT 到達光子数などが記録されます。
6. Run 終了時に `../root/output<RunID>.root` へ ROOT ファイルが出力されます。

## 現在の検出器設定

`src/DetectorConstruction.cc` の `Mode` マップで、各検出器の有効・無効、ID、個数が管理されています。

現在のテスト用設定では、以下の状態です。

- `UROKO`: 有効、1 台
- `LigGlass`: 無効
- `HILE`: 無効
- `HPGe`: 無効
- `BetaPlastic`: 無効
- `Magnet`: 無効
- `Frame`: 無効
- `Floor`: 無効
- `Shield`: 無効
- `Chamber`: 無効
- `Stopper`: 無効

つまり、現在は UROKO 単体テスト用の設定になっています。

## UROKO 検出器

UROKO の形状と光学設定は主に `src/LogVol/UROKOLogVol.cc` に実装されています。

構成要素は以下です。

- Mother volume
  - 真空の箱です。
- Bumper
  - シンチレータ前面に置かれた極薄の真空層です。
  - 光学境界を明示的に設定するために使われています。
- Scintillator
  - BC408 材料の六角柱シンチレータです。
- Light Guide
  - アクリル製のライトガイドです。
- PMT
  - PMT ガラスで作られた volume が 2 個配置されています。
- Cathode
  - PMT 内部に配置される光電面相当の volume です。

光学表面は以下のように設定されています。

- Scintillator skin: 鏡面反射
- Light guide skin: 鏡面反射
- Cathode skin: 全吸収、量子効率 100% と仮定
- Scintillator 前面と Bumper の境界: Tybek surface
- Scintillator と Light guide の境界: dielectric surface
- Light guide と PMT の境界: dielectric surface

## 材料定義

材料は `src/Material/` 以下に分離されています。

主な材料は以下です。

- `BC408Mat`
  - BC408 シンチレータを定義します。
  - 屈折率、吸収長、発光スペクトル、シンチレーション収量、時定数、立ち上がり時間、Birks 定数を設定しています。
  - 光子発生数は `10000 / MeV` に設定されています。

- `GS20Mat`
  - Li-glass シンチレータ GS20 を定義します。
  - 95% 濃縮 `6Li` を含む Li ガラス組成を明示的に構築しています。

- `AcrylicMat`, `PMTGlassMat`, `VacuumMat`, `AirMat`, `MgOMat`, `NeomaxMat`
  - 各検出器や光学輸送で使う材料です。

## 物理リスト

`src/PhysicsList.cc` では `FTFP_BERT_HP` をベースにしています。

追加で登録されている物理は以下です。

- `G4RadioactiveDecayPhysics`
- `G4EmLivermorePhysics`
- `G4OpticalPhysics`

これにより、中性子、放射性崩壊、低エネルギー電磁過程、光学光子の取り扱いを含むシミュレーションになっています。

## 粒子生成

粒子生成は `src/PrimaryGenerator.cc` と `src/PrimaryGeneratorMessenger.cc` で実装されています。

`/mygen/sourceType` コマンドで線源タイプを切り替えられます。

候補は以下です。

- `neutron`
- `gamma`
- `gamma(137Cs)`
- `137Cs`
- `90Sr`
- `90Y`

デフォルトは `neutron` です。

現在の実装では、デフォルト neutron は以下の設定です。

- 粒子: neutron
- エネルギー: 1.00 MeV
- 位置: 原点
- 方向: ランダム方向

放射性同位体を選んだ場合は、Geant4 の IonTable から該当核種を生成し、静止状態から崩壊させる想定です。

## SensitiveDetector と記録内容

現在使われている SensitiveDetector は以下です。

- `ScintiSD`
  - シンチレータでのエネルギー付与を記録します。
  - 発生した optical photon 数を数えます。
  - 最初の hit 位置を global/local 座標で保存します。
  - hit 数を数えます。

- `CathodeSD`
  - PMT 光電面に到達した optical photon を数えます。
  - PMT1 と PMT2 を copy number から区別します。
  - 到達時刻と到達位置を `EventAction` に渡します。
  - 到達した optical photon は `fStopAndKill` で停止します。

`DetectorConstruction::ConstructSDandField()` では、現在 UROKO の以下の volume に SensitiveDetector が割り当てられています。

- UROKO scintillator: `ScintiSD`
- UROKO PMT volume: `CathodeSD`

## ROOT 出力

`src/RunAction.cc` で `G4AnalysisManager` の ntuple が作られます。

出力ファイル名は以下です。

```text
../root/output<RunID>.root
```

主な ntuple column は以下です。

- `Scinti_Edep`
- `Scinti_Photons`
- `Scinti_Hits`
- `Scinti_HitPos_Global`
- `Scinti_HitPos_Local`
- `Scinti_HitPos_Radius`
- `PMT1_Photons`
- `PMT1_Efficiency`
- `PMT2_Photons`
- `PMT2_Efficiency`
- `PMT1_HitTimes`
- `PMT1_HitPos_X`
- `PMT1_HitPos_Y`
- `PMT1_HitPos_Z`
- `PMT2_HitTimes`
- `PMT2_HitPos_X`
- `PMT2_HitPos_Y`
- `PMT2_HitPos_Z`

PMT efficiency は、イベントごとに以下で計算されています。

```text
PMT photons / generated scintillation photons
```

## SteppingAction の役割

`src/SteppingAction.cc` では、光学光子が境界などでほぼ移動しないステップを繰り返した場合に、トラックを強制停止します。

目的は、光学光子が volume の角などで捕まり、Geant4 側の abort につながる問題を避けることです。

現在は、光学光子の step length が `1e-12 mm` 以下の状態を連続して検出し、一定回数に達したら `fStopAndKill` します。

## 現在のコードの特徴

- Geant4 の検出器テスト用コードとして、検出器ごとの LogicalVolume が分離されています。
- UROKO の光学輸送をかなり詳細に扱っています。
- 材料、光学表面、検出器形状がファイル単位で整理されています。
- ROOT ntuple 出力により、シンチレータでの発光量、PMT 到達光子数、hit 位置、到達時刻を解析できます。
- 本番配置とテスト配置がコメントで共存しており、現在は UROKO 1 台テスト用の状態です。
- Optical photon の境界トラップ対策が `ScintiSD` と `SteppingAction` に入っています。

## 注意点

- `sim.cc` は常に UI セッションを開始する実装になっており、現在コメントアウトされている batch 実行分岐は使われていません。
- `DetectorConstruction.cc` の `Mode` はソースコード内のグローバルなマップで切り替える形式です。マクロから切り替える仕組みは現状見当たりません。
- `CathodeSurface` は量子効率 100% の理想的な全吸収面として扱われています。実 PMT の量子効率を反映するには変更が必要です。
- 現状コメントにもある通り、各検出器を一台ずつテストする想定が強く、複数検出器同時運用時には SensitiveDetector や ID 管理の確認が必要です。
- 未コミットの変更が多数ある状態なので、現在の内容は作業中のコードを前提にした要約です。
