# `G4EmSaturation` による可視エネルギー集計の実装計画

## 1. 目的

`ScintiSD` が現在保存している全エネルギー付与 `Scinti_Edep` に加えて、BC408 の Birks 消光を考慮した可視エネルギーをイベントごとに保存する。

追加する物理量は、まず `G4EmSaturation::VisibleEnergyDepositionAtAStep()` の戻り値のイベント内総和とする。出力名は、電子較正をまだ行っていない値を厳密な `MeVee` と誤認しないよう、`Scinti_Evis` とする。

```text
各 step:
    edep = GetTotalEnergyDeposit()
    evis = G4EmSaturation::VisibleEnergyDepositionAtAStep(step)

各 event:
    Scinti_Edep = sum(edep)
    Scinti_Evis = sum(evis)
```

`Scinti_Evis` を最終的に電子等価エネルギー `MeVee` として扱う場合は、後述する電子またはガンマ線による較正を別途行う。

## 2. 現状確認

### `ScintiSD`

- `Initialize()` でイベント集計値を初期化している。
- `ProcessHits()` で `GetTotalEnergyDeposit()` を `fTotalEdep` に加算している。
- `EndOfEvent()` で `AnalysisOutput::FillScinti()` にイベント集計値を渡している。
- `edep == 0` の step は早期 return している。

この構造は、step ごとの `evis` を新しいメンバへ加算する実装に適している。

### BC408 の Birks 定数

`src/Material/BC408Mat.cc` には、すでに次の設定が存在する。

```cpp
fMaterial->GetIonisation()->SetBirksConstant(
    (1.31e-2 * (g/cm2/MeV)) / (1.032 * (g/cm3))
);
```

これは約 `0.1269 mm/MeV` であり、`G4EmSaturation` が材料から読み取る値である。したがって、今回の実装で別の Birks 定数を `ScintiSD` に重複設定しない。

ただし、この数値が今回再現したい BC408 の測定条件に適切かは、物理検証時に確認する。

### 光学過程との関係

`G4Scintillation` も、Birks 定数が設定されている場合には内部で `G4EmSaturation` の可視エネルギーを発生光子数へ反映する。今回 `ScintiSD` で `evis` を計算する目的は「記録」であり、光子生成へ追加の重みを掛けない。

したがって、`fGeneratedPhotons` や各 optical photon の weight を `evis/edep` で再補正しない。再補正すると Birks 消光を二重適用する可能性がある。

## 3. 変更対象

実装時に変更するファイルは次の4つとする。

```text
include/ScintiSD.hh
src/ScintiSD.cc
include/AnalysisOutput.hh
src/AnalysisOutput.cc
```

`EventAction` は現在の出力経路で変更不要である。Birks 定数も既に設定済みなので、原則として `BC408Mat.cc` は変更しない。

## 4. 実装手順

### Step 1: `ScintiSD` にイベント可視エネルギーを追加する

`include/ScintiSD.hh` の private メンバに次を追加する。

```cpp
G4double fTotalEvis;
```

必要なら確認用 getter も追加できる。

```cpp
G4double GetTotalEvis() const { return fTotalEvis; }
```

getter は現状の出力経路には必須ではないが、`EventAction` やデバッグ出力から値を比較するときに有用である。

### Step 2: イベント開始時に値を初期化する

`ScintiSD::Initialize()` に次を追加する。

```cpp
fTotalEvis = 0.0;
```

コンストラクタの初期化リストにも `fTotalEvis(0.0)` を追加し、初回イベント以前にも未初期化値を持たないようにする。

### Step 3: `G4EmSaturation` を利用するための include を追加する

`src/ScintiSD.cc` に次を追加する。

```cpp
#include "G4EmSaturation.hh"
#include "G4LossTableManager.hh"
```

`G4EmSaturation` を `new` して `ScintiSD` が所有する設計にはしない。Geant4 の EM 管理クラスが提供するスレッドごとのインスタンスを使用する。

### Step 4: 各 step の可視エネルギーを計算する

`ProcessHits()` のエネルギー付与処理を、概念的に次の形へ拡張する。

```cpp
const G4double edep = aStep->GetTotalEnergyDeposit();
if (edep <= 0.0) return false;

auto* saturation =
    G4LossTableManager::Instance()->EmSaturation();

const G4double evis =
    saturation->VisibleEnergyDepositionAtAStep(aStep);

fTotalEdep += edep;
fTotalEvis += evis;
```

実装上の判断:

- `evis` はイベント総 `edep` に対して一度だけ計算せず、必ず各 step で計算してから加算する。Birks 応答は step の `edep/stepLength` に対して非線形だからである。
- `GetNonIonizingEnergyDeposit()`、粒子種、材料、step 長は `VisibleEnergyDepositionAtAStep()` 内部で取得されるため、`ScintiSD` 側で Birks 式を再実装しない。
- 現状どおり `edep == 0` の step を除外してよい。Geant4 の実装も `edep <= 0` に対して `0` を返す。
- 防御的実装にする場合は `saturation != nullptr` を確認し、null 時は警告を出して `evis = edep` とする。ただし、通常の EM physics 初期化後には管理インスタンスから取得できる想定である。

`EmSaturation()` の取得をコンストラクタで行わず、最初は `ProcessHits()` 内で行う方針を推奨する。`ConstructSDandField()` は MT 実行時に worker ごとに呼ばれる一方、物理初期化との順序をクラスの所有関係に埋め込まずに済むためである。性能上問題になることを計測した場合のみ、worker の `ScintiSD` が持つ非所有ポインタとしてキャッシュする案を検討する。

### Step 5: `AnalysisOutput` の列定義を追加する

`include/AnalysisOutput.hh` に列IDを追加する。

```cpp
G4int fScintiEvis = -1;
```

また、`FillScinti()` の引数へ `evis` を追加する。

```cpp
void FillScinti(
    G4double edep,
    G4double evis,
    G4double generatedPhotons,
    G4int hitCount
);
```

`src/AnalysisOutput.cc` の `Book()` では、`Scinti_Edep` の直後に新しい double column を作る。

```cpp
fScintiEvis =
    analysisManager->CreateNtupleDColumn("Scinti_Evis");
```

列IDは戻り値で管理されているため、途中へ列を追加しても既存の固定番号を書き換える必要はない。

### Step 6: 出力値を受け渡す

`AnalysisOutput::FillScinti()` で次を追加する。

```cpp
analysisManager->FillNtupleDColumn(fScintiEvis, evis);
```

`ScintiSD::EndOfEvent()` の呼び出しは次の4量を渡す形へ変更する。

```cpp
output->FillScinti(
    fTotalEdep,
    fTotalEvis,
    fGeneratedPhotons,
    fHitCount
);
```

単位は既存の `Scinti_Edep` と同じ Geant4 内部エネルギー単位で保存される。このプロジェクトでは内部単位のエネルギーは MeV なので、ROOT 側の列名または解析資料に `[MeV]` と明記する。

## 5. 推奨する実装後の処理フロー

```text
BC408Mat
  └─ 材料へ Birks constant を設定

ScintiSD::Initialize
  ├─ fTotalEdep = 0
  └─ fTotalEvis = 0

ScintiSD::ProcessHits（各 step）
  ├─ edep を取得
  ├─ G4EmSaturation から evis を計算
  ├─ fTotalEdep += edep
  └─ fTotalEvis += evis

ScintiSD::EndOfEvent
  └─ AnalysisOutput::FillScinti(edep, evis, photons, hits)

AnalysisOutput
  ├─ Scinti_Edep を保存
  └─ Scinti_Evis を保存
```

## 6. 検証計画

### 6.1 出力構造の確認

- ROOT tree に `Scinti_Evis` が1列追加される。
- event ごとに1回だけ値が記録される。
- `Scinti_Edep`、`Scinti_Photons`、`Scinti_Hits` の既存値と列対応が崩れていない。

### 6.2 基本的な数値条件

エネルギー付与がある event について次を確認する。

```text
0 <= Scinti_Evis <= Scinti_Edep
```

Birks 定数を正値に設定した通常の step では `Evis` が `Edep` を超えない。違反がある場合は、列IDの取り違え、イベント初期化漏れ、単位の誤りを優先して調べる。

### 6.3 Birks 定数に対する応答

物理検証用の比較 run として、次を確認する。

1. 現在の Birks 定数で `Evis/Edep` を取得する。
2. 検証用に Birks 定数を `0` にした条件では、`Evis` と `Edep` が一致することを確認する。
3. Birks 定数を大きくすると、特に高 `dE/dx` 粒子で `Evis/Edep` が低下することを確認する。

本番コードの定数を恒久的に書き換えるのではなく、検証条件として明示的に管理する。

### 6.4 粒子種による比較

同程度の付与エネルギーを持つ電子と陽子または反跳核について、`Evis/Edep` を比較する。高 `dE/dx` の粒子でより強い消光が現れることを期待する。

確認用の派生量:

```text
quenching_factor = Scinti_Evis / Scinti_Edep
```

`Scinti_Edep == 0` の event では除算しない。

### 6.5 step-size 依存性

Birks 補正は step ごとの `edep/stepLength` を使うため、`G4UserLimits` や production cut を変えた比較で `Scinti_Evis` が十分収束することを確認する。現在、シンチレータ論理ボリュームには `fStepLimit` が設定されているため、その値も検証記録へ残す。

### 6.6 発生光子数との整合性

`SCINTILLATIONYIELD = 10000/MeV` なので、多数イベントの平均では概ね次の相関を期待する。

```text
GeneratedPhotons ≈ Scinti_Evis × SCINTILLATIONYIELD
```

ただし `RESOLUTIONSCALE` による統計揺らぎがあるため、単一イベントでの完全一致を要求しない。また、secondary の数え方、track の打ち切り、光学過程の設定も差に影響し得る。

## 7. 電子等価エネルギーへの拡張

`Scinti_Evis` は Birks 補正済みの可視エネルギーだが、実験装置としての厳密な `MeVee` は較正で定義する。

推奨手順:

1. 既知エネルギーの単色電子、または既知の Compton edge・全吸収ピークを持つガンマ線をシミュレーションする。
2. 実験で使用する観測量を決める。候補は `Scinti_Evis`、発生光子数、PMT 到達光子数である。
3. 電子エネルギーと平均応答の関係をフィットする。
4. 得られた較正式を解析段階で適用し、別名の `Energy_ee` として扱う。

光輸送・PMT 収集効率まで含めた検出器応答を `MeVee` にしたい場合は、`Scinti_Evis` ではなく PMT 到達光子数または光電子数を較正対象にする。

## 8. Geant4 example の参照先

Geant4 11.2.2 では、次の example が参考になる。

- `examples/advanced/amsEcal/src/SteppingAction.cc`
  - Birks 補正を step ごとに直接計算する例。
- `examples/advanced/amsEcal/src/DetectorConstruction.cc`
  - 材料への `SetBirksConstant()` 設定例。
- `examples/extended/electromagnetic/TestEm3`
  - エネルギー付与と Birks 応答の扱いを確認できる。
- `examples/extended/optical/OpNovice2`
  - optical scintillation と Birks 定数の設定例。

今回の実装では example の手計算式をコピーせず、粒子種や非電離エネルギー損失も扱う `G4EmSaturation::VisibleEnergyDepositionAtAStep()` を使用する。

## 9. 完了条件

- `Scinti_Edep` を保持したまま `Scinti_Evis` が追加されている。
- `fTotalEvis` がイベント開始時に必ずゼロへ戻される。
- 各 step に対して `VisibleEnergyDepositionAtAStep()` を1回だけ呼び、その結果をイベント内で加算している。
- Birks 定数を `ScintiSD` 内で重複設定していない。
- optical photon 数へ追加の Birks 補正を掛けていない。
- `0 <= Evis <= Edep`、Birks 定数ゼロ時の `Evis == Edep`、粒子種依存性を確認できている。
- `Scinti_Evis` と較正後の `MeVee` を用語として区別している。

