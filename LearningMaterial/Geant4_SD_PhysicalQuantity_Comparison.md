# Geant4 Sensitive Detector における物理量取得ロジックの比較

## 目的

この資料は、過去の先輩が作成したコードと現在の `ScintiSD`・`CathodeSD` を比較し、シンチレータおよびPMTに関する物理量の取得ロジックを整理したものです。

比較対象は主に次のファイルです。

### 先輩コード

プロジェクトルート:

```text
/home/hashizume/Geant4-project/Neutron201005-20260718T063218Z-1-001/Neutron201005/source
```

主に確認したファイル:

- `src/SensitiveDetectors.cc`
- `src/SteppingAction.cc`
- `src/Geometry.cc`
- `src/RunAction.cc`
- `include/SensitiveDetectors.hh`
- `include/myParameter.hh`

### 現行コード

主に確認したファイル:

- `src/ScintiSD.cc`
- `src/CathodeSD.cc`
- `src/DetectorConstruction.cc`
- `src/EventAction.cc`
- `src/AnalysisOutput.cc`
- `src/Material/BC408Mat.cc`
- `src/Surface/UROKO/CathodeSurface.cc`

先輩コードのディレクトリには、`Geometry_org.cc`、`Geometry_beta.cc`、`with_pla/`、`without_beta-pla/` などの派生版も残っています。そのため、「先輩が実際に計算に使用したファイル」がどれかによって、一部の判定、特にPMTのSensitive Detectorの付与先に関する判定が変わる可能性があります。

この資料では、ファイル名上の本流と考えられる直下の `SensitiveDetectors.cc`、`SteppingAction.cc`、`Geometry.cc`、`RunAction.cc` を基準にしています。

## 最初に区別するべき物理量

シンチレータからPMTまでの処理は、次のような段階に分けて考える必要があります。

```text
全エネルギー付与 Edep
        ↓ Birks 消光
可視エネルギー Evis
        ↓ scintillation yield と統計揺らぎ
シンチレータ内で生成された光子数 N_generated
        ↓ 吸収・反射・屈折・幾何学的損失
光電面へ到達した光子数 N_arrived
        ↓ 量子効率 EFFICIENCY
光電面で検出された光子数 N_detected
```

これらは別々の量です。

- `Edep` は、粒子が物質へ実際に付与したエネルギーです。
- `Evis` は、Birks消光を考慮した可視エネルギーです。
- `N_generated` は、シンチレータ内で生成されたoptical photon数です。
- `N_arrived` は、輸送後に光電面まで到達した光子数です。
- `N_detected` は、光電面の量子効率を通過し、`G4OpBoundaryProcess` が `Detection` と判定した光子数です。

コード中に `photon` や `hit` という名前があっても、どの段階の量を意味しているかを確認しなければなりません。

## 比較結果の要約

| 対象 | 先輩コード | 現行コード | 評価 | より推奨される実装 |
|---|---|---|---|---|
| 全エネルギー付与 | `GetTotalEnergyDeposit()` を加算 | 同じ関数を加算 | ほぼ同等。現行の責務分離が分かりやすい | SD内で、対象logical volumeの全stepをイベント単位で加算する |
| 可視エネルギー | `SteppingAction` でpost volumeを判定 | `ScintiSD` 内で取得 | 現行が優れる | `ScintiSD` 内で `G4LossTableManager` の `G4EmSaturation` を使用する |
| 生成光子数 | 全二次粒子を無条件に数える | optical photonだけ数える | 現行が優れるが不完全 | `StackingAction` などでcreator processを調べ、`Scintillation` と `Cerenkov` を分ける |
| 最初のヒット | primaryの最初のエネルギー減少 | 最初に処理された非ゼロedep step | 目的次第であり、どちらも完全ではない | 入射点、最初のedep、最初の相互作用を別々に定義する |
| PMT到達数 | 意図はPMTからCathodeへの到達数だが、SD付与先と不整合 | PMTにSDを付けてCathode境界を判定 | 到達数としては現行が優れる | 到達数と `Detection` 数を別々に記録する |
| PMT検出数 | `Detection` statusを確認しない | `Detection` statusを確認しない | どちらも量子効率変更に弱い | `G4OpBoundaryProcessStatus::Detection` のときだけ検出数を加算する |
| PMT時刻 | 最小時刻だけ保存 | 全光子の時刻をvector保存 | 現行が優れる | 全時刻を保存し、必要なら最小時刻を別途計算する |
| イベント選別 | `En > 0` のイベントだけ保存 | 全イベントを保存 | 現行が優れる | 全イベントを保存し、入射・edep・検出フラグで後から選別する |
| 集計構造 | グローバルvectorへ直接保存 | SDとEventActionのイベントメンバで管理 | 現行が優れる | 詳細なhitには `G4THitsCollection` を使い、EventActionで回収・出力する |

## 一致している点

### イベント単位で初期化・加算している

両方とも、イベント開始時に値を初期化し、`ProcessHits()` または `SteppingAction` でstepごとに加算し、イベント終了時にROOTへ保存しています。

### `Edep` は全粒子の寄与を含む

どちらのコードも、`GetTotalEnergyDeposit()` を呼ぶ前に粒子種を限定していません。

したがって、保存されるエネルギー付与には、例えば次の寄与が含まれます。

- 反跳陽子
- 電子
- gammaの二次反応
- neutron反応で生成された荷電粒子
- その他、シンチレータ内でエネルギーを付与した全粒子

これは「中性子自身が落としたエネルギー」ではなく、「シンチレータへ付与された全エネルギー」です。

### Birks消光に同じGeant4 APIを使用している

両コードとも、最終的には次の関数を使っています。

```cpp
G4EmSaturation::VisibleEnergyDepositionAtAStep(step)
```

ただし、呼び出す場所と対象stepの選び方が異なります。

### PMTではoptical photonを選別している

先輩コード、現行コードとも、PMTに関してはoptical photonだけを対象にしています。

### 明示的な単位変換をしていない

ROOTへ保存する前に `/ MeV`、`/ ns`、`/ mm` などの変換をしていません。そのため、Geant4内部単位の数値がそのまま保存されます。

通常は次のように解釈できます。

- エネルギー: MeV
- 時刻: ns
- 位置: mm
- 光子数・step数: 無次元の個数

ROOTのbranch自体には単位情報が付かないため、branch名または解析資料に単位を明記することが推奨されます。

## 1. 全エネルギー付与 `Edep`

### 先輩コード

`src/SensitiveDetectors.cc:119` で、シンチレータのcopy numberが2のときに次を実行しています。

```cpp
Scinti_E[0] += aStep->GetTotalEnergyDeposit();
```

### 現行コード

`src/ScintiSD.cc:44` でエネルギー付与を取得し、ゼロなら処理を終了します。

```cpp
const G4double edep = aStep->GetTotalEnergyDeposit();
if (edep == 0.) return false;

fTotalEdep += edep;
```

ゼロを加算しても合計値は変わらないため、`Edep` の総和だけを考えれば両者はほぼ同じです。

### どちらが優れているか

現行コードの方が推奨されます。

理由は、`ScintiSD` がシンチレータlogical volumeへ直接付与されており、「どのvolumeの値か」がSDの付与先によって決まるためです。先輩コードのようにcopy number `2` を処理内へ埋め込む方法は、geometry変更でcopy numberが変わると取得できなくなります。

### 推奨実装

- `ScintiSD` を対象logical volumeへ付与する。
- SD内では `GetTotalEnergyDeposit()` をイベント単位で加算する。
- 粒子別の内訳が必要な場合は、総和を置き換えるのではなく、粒子分類ごとの追加集計を行う。
- `edep > 0` のstep数を数える場合は、物理的なhit数ではなく `EdepStepCount` など、意味が分かる名前にする。

## 2. 可視エネルギー `Evis`

### 先輩コード

可視エネルギーはSDではなく、`src/SteppingAction.cc:31` で計算しています。

```cpp
volumeID = aStep->GetPostStepPoint()->GetTouchableHandle()->GetCopyNumber();

if (volumeID == 2) {
    Scinti_E_ee[0] +=
        fSaturationEngine->VisibleEnergyDepositionAtAStep(aStep);
}
```

判定にpost-step側のvolumeを使っています。

stepは基本的にpre-step側のvolumeに属するため、この方法では次の問題が起こり得ます。

- シンチレータから外へ出る最後のstepを取りこぼす。
- 外部volumeからシンチレータへ入る境界stepを、シンチレータのstepとして選ぶ。
- `Edep` と `Evis` でvolume選択条件が揃わない。

### 現行コード

`src/ScintiSD.cc:48` で、`Edep` と同じstepに対して可視エネルギーを計算しています。

```cpp
auto* saturation = G4LossTableManager::Instance()->EmSaturation();
const G4double evis =
    saturation->VisibleEnergyDepositionAtAStep(aStep);

fTotalEvis += evis;
```

### どちらが優れているか

現行コードの方が明確に優れています。

- `Edep` と `Evis` の対象stepが一致する。
- 対象volumeがSDの付与先で決まる。
- `G4EmSaturation` を独自に `new` せず、Geant4の管理インスタンスを利用している。
- MT実行時の所有関係が分かりやすい。

### 推奨実装

現行方式を維持し、次を明確にします。

- `Evis` は校正済みの厳密な `MeVee` とは限らない。
- 材料へ設定したBirks定数を `G4EmSaturation` が使用する。
- `Evis` を記録した後、発生光子へさらに `Evis/Edep` を掛けない。Geant4のscintillation過程ですでにBirks消光が反映されている場合、二重補正になるためです。

## 3. シンチレータ内の生成光子数

### 先輩コード

`src/SensitiveDetectors.cc:120` では、次の値をそのまま加算しています。

```cpp
Scinti_phot[0] += aStep->GetNumberOfSecondariesInCurrentStep();
```

この関数が返すのは「そのstepで生成された全二次粒子数」です。

そのため、`photon` branchには次の粒子も含まれる可能性があります。

- 反跳陽子
- gamma
- electron
- ion
- optical photon以外の二次粒子

したがって、branch名が `photon` でも、厳密には光子数ではありません。

### 現行コード

`src/ScintiSD.cc:55` では、二次粒子を走査し、optical photonだけを数えています。

```cpp
auto secondaries = aStep->GetSecondaryInCurrentStep();

for (auto track : *secondaries) {
    if (track->GetDefinition()
        == G4OpticalPhoton::OpticalPhotonDefinition()) {
        fGeneratedPhotons++;
    }
}
```

### どちらが優れているか

現行コードの方が優れています。

少なくとも、数えている対象がoptical photonに限定されているためです。

ただし、現行コードにも次の注意点があります。

1. creator processを確認していないため、`Scintillation` と `Cerenkov` を区別していない。
2. 光子カウントが `edep == 0` の早期returnより後にあるため、ゼロedep stepから生成されたoptical photonを数えない。
3. 個数であるにもかかわらず、`G4double` とROOTのDColumnで保存している。

### より推奨される実装

「シンチレーション生成光子数」を求める場合は、optical photonであることに加えてcreator processを確認します。

概念的には次の条件です。

```cpp
track->GetDefinition()
    == G4OpticalPhoton::OpticalPhotonDefinition()

track->GetCreatorProcess() != nullptr

track->GetCreatorProcess()->GetProcessName()
    == "Scintillation"
```

用途に応じて、少なくとも次を別々に数えることが推奨されます。

- `NScintillationPhotons`
- `NCerenkovPhotons`
- `NOtherOpticalPhotons`

公式の `extended/optical/LXe` exampleでは、`LXeStackingAction` が新しく生成されたoptical photonのcreator processを確認し、Scintillation光とCerenkov光を分けて数えています。

全optical photonを生成時に一度だけ分類する目的では、この `StackingAction` 方式が最も明確です。さらに「シンチレータ内で生成された光子」だけを分母にする場合は、生成volumeまたはvertex volumeの条件も揃える必要があります。

## 4. シンチレータの「最初のヒット」

### 先輩コード

`src/SensitiveDetectors.cc:95` では、Track ID 1に限定し、次の条件を満たすstepのうちglobal timeが最小のものを選びます。

```cpp
aStep->GetDeltaEnergy() < 0
```

保存している位置と時刻はpost-step側です。

```cpp
Scinti_T[0] = aStep->GetPostStepPoint()->GetGlobalTime();
Scinti_hitpos[...] = aStep->GetPostStepPoint()->GetPosition();
```

`GetDeltaEnergy() < 0` は、現在trackしている粒子のエネルギーが減ったことを意味します。`GetTotalEnergyDeposit() > 0` と同じ条件ではありません。エネルギーが二次粒子へ移っただけの場合なども含み得ます。

### 現行コード

`src/ScintiSD.cc:65` では、最初に処理された `edep > 0` のstepについて、pre-step位置を保存しています。

```cpp
if (fHitCount == 0) {
    fFirstHitPosGlobal =
        aStep->GetPreStepPoint()->GetPosition();
}
```

現行コードは全粒子を対象とし、global timeの最小値は比較していません。

### どちらが優れているか

一概には決められません。

- 「primaryが最初にエネルギーを失った時刻」を求めたいなら、先輩コードの時間比較には意味がある。
- 「最初に実際のエネルギー付与が発生した場所」を求めたいなら、現行の `edep > 0` の方が意味に近い。
- 現行コードのlocal座標保存は、複数方向へ配置した検出器を比較するうえで優れている。
- 現行コードの「最初」はtrack処理順に依存し、必ずしもglobal timeが最小とは限らない。

したがって、どちらも「first hit」という1つの曖昧な名前へ複数の意味を含めている点が問題です。

### より推奨される実装

目的別に量を分けます。

#### 検出器への入射点

- pre-step statusが `fGeomBoundary` のstepを使う。
- pre-step位置、時刻、運動エネルギーを保存する。
- primaryだけを対象にするか、全粒子を対象にするかを明記する。

#### 最初のエネルギー付与位置

- `GetTotalEnergyDeposit() > 0` を条件にする。
- global timeの最小値を明示的に比較する。
- 時刻と位置を必ず同じstepから保存する。

#### step内の代表位置

連続的なエネルギー付与に対しては、pre位置またはpost位置だけでなく、次の中点を代表位置とする方法があります。

```cpp
const auto pos =
    (prePoint->GetPosition() + postPoint->GetPosition()) / 2.0;
```

公式の `extended/optical/LXe/src/LXeScintSD.cc` は、この中点をhit位置として使用しています。

#### 物理相互作用位置

- `GetPostStepPoint()->GetProcessDefinedStep()` を確認する。
- `hadElastic`、`neutronInelastic`、`nCapture` など、目的のprocessを明示的に選ぶ。

## 5. `Scinti_Hits` の意味

現行コードの `Scinti_Hits` は、`edep > 0` だった `ProcessHits()` 呼び出し回数です。

これは次のいずれとも限りません。

- 入射粒子数
- 中性子反応数
- optical photon数
- 独立した検出イベント数

1つの荷電粒子が複数stepでエネルギーを付与すれば、複数hitとして数えられます。

### どちらが優れているか

先輩コードには対応する量がないため、情報量としては現行コードが優れています。ただし、`Hits` という名前は意味が広すぎます。

### 推奨実装

目的に合わせて名前と条件を分けます。

- 非ゼロedep step数なら `Scinti_EdepStepCount`
- シンチレータへ入射したtrack数なら、境界入射時の一意なTrack IDを数える
- hadronic interaction数なら、process名を確認して数える
- イベントにedepがあったかだけなら、booleanの `HasScintiEdep`

## 6. PMT到達光子数

### 先輩コード

`src/SensitiveDetectors.cc:122` と `:139` では、次の条件を想定しています。

```text
PMT1: pre copy number 4 → post copy number 5
PMT2: pre copy number 6 → post copy number 7
```

つまり、PMTガラスから娘volumeであるCathodeへ向かうoptical photonを数える意図です。

しかし、基準とした `src/Geometry.cc:269` 付近では、PMTへのSD付与がコメントアウトされ、CathodeへSDが付与されています。

```cpp
// LV_PMT->SetSensitiveDetector(MySensitiveDetectors);
LV_PMT_Cathode->SetSensitiveDetector(MySensitiveDetectors);
LV_PMT_Cathode2->SetSensitiveDetector(MySensitiveDetectors);
```

通常、`ProcessHits()` はpre-step側のsensitive volumeに対して呼ばれます。Cathode内で呼ばれた場合、pre copy numberは5または7なので、case 4または6へ入りません。

したがって、この `Geometry.cc` と `SensitiveDetectors.cc` の組み合わせでは、PMT hitが0のままになる可能性が高いです。

ただし、`src/Geometry_org.cc:170` ではPMTへSDを付与しています。先輩がこの版を使用していたなら、少なくともPMT1のロジックは付与先と整合します。

### 現行コード

現行の `CathodeSD` はPMTガラスlogical volumeへ付与されています。

`src/CathodeSD.cc:26` 以降で、次の条件を確認しています。

1. optical photonである。
2. post-step statusが `fGeomBoundary` である。
3. post-step側physical volume名が `Cathode` である。
4. pre-stepのPMT copy numberが1または2である。

この条件を満たした場合、PMT別に到達光子数を加算します。

### どちらが優れているか

「Cathode境界へ到達した光子を数える」という目的では、現行コードの方が優れています。

- SDの付与先とpre/post判定が整合している。
- `fGeomBoundary` を明示的に確認している。
- 同じPMT logical volumeをcopy number 1/2で識別している。
- PMT別の時刻と位置も保存できる。

### ただし、到達数と検出数は異なる

現行コードは `G4OpBoundaryProcess` のstatusが `Detection` かどうかを確認していません。

現在のCathode surfaceは次の理想条件です。

```text
REFLECTIVITY = 0
EFFICIENCY   = 1
```

この条件では、Cathodeへ到達した光子を検出光子とみなしても結果は一致しやすくなります。しかし将来、実際の量子効率を設定すると、到達数と検出数は一致しません。

さらに、現行コードは到達した光子を無条件に `fStopAndKill` します。反射率を0より大きく変更した場合、本来反射する光子まで停止させる可能性があります。

### より推奨される実装

次の2量を分けて記録します。

```text
N_arrived  = Cathode境界へ到達したoptical photon数
N_detected = 境界statusがDetectionとなった光子数
```

検出数については、次のいずれかの公式exampleに近い方式が推奨されます。

1. `G4OpBoundaryProcess::GetStatus()` を確認し、`Detection` のときだけPMT hitを作る。
2. `G4OpticalParameters::SetBoundaryInvokeSD(true)` を設定し、boundary processからSDを起動させる。

`extended/optical/LXe` は1の方式、`extended/optical/wls` は2の方式を採用しています。

## 7. PMTの時刻と位置

### 先輩コード

PMTごとに、global timeが最小の光子1個だけを保存します。

### 現行コード

PMTごとに、全到達光子について次をvector保存します。

- global time
- global x
- global y
- global z

### どちらが優れているか

現行コードの方が優れています。

全光子の情報があれば、解析段階で次の量を計算できます。

- 最初の光子時刻
- 平均到達時刻
- 時間分布
- time spread
- 光電面上の位置分布
- PMT1とPMT2の時間差

一方、最小時刻だけを保存すると、後から時間分布を復元できません。

### 推奨実装

- 全光子の時刻をvector保存する。
- vectorの格納順を時刻順と仮定しない。
- first photon timeが頻繁に必要なら、vectorとは別に最小global timeも保存する。
- 検出面の形状比較に使う場合は、global位置に加えてPMT local位置も保存する。
- 必要なら光子エネルギー、波長、creator processも保存する。

## 8. 効率の定義

先輩コードには効率branchがなく、光子数から解析時に導出する設計です。

現行コードは、PMT別に次を計算しています。

```text
PMT1_Efficiency = PMT1_Photons / Scinti_Photons
PMT2_Efficiency = PMT2_Photons / Scinti_Photons
```

ただし `src/AnalysisOutput.cc:118` の `PMT_SUM_Efficiency` は次の計算です。

```text
(PMT1_Efficiency + PMT2_Efficiency) / 2
```

これは合計効率ではなく、2本のPMTの平均効率です。

合計到達効率を意図する場合は、数学的には次です。

```text
PMT_SUM_Efficiency
    = (PMT1_Photons + PMT2_Photons) / Scinti_Photons
    = PMT1_Efficiency + PMT2_Efficiency
```

### どちらが優れているか

効率をイベントごとに明示している点では現行コードが便利です。しかし、現在の `PMT_SUM_Efficiency` は名前と定義が一致していないため、そのままでは推奨できません。

また、分母はシンチレータ内で生成されたoptical photon、分子はCathodeへ到達した全optical photonです。ガイドやPMTガラスでCerenkov光が生成される場合などは、光子の起源が揃わない可能性があります。

### より推奨される実装

効率の段階を名前で区別します。

```text
CollectionEfficiency
    = N_arrived_from_scintillator / N_generated_in_scintillator

DetectionEfficiency
    = N_detected_from_scintillator / N_generated_in_scintillator

PMT1CollectionEfficiency
    = N_arrived_PMT1 / N_generated_in_scintillator

PMT2CollectionEfficiency
    = N_arrived_PMT2 / N_generated_in_scintillator
```

分子と分母で、次の条件を揃える必要があります。

- 光子の生成volume
- creator process
- 波長範囲
- track weight
- イベント選別条件

## 9. イベント選別

### 先輩コード

`src/SensitiveDetectors.cc:70` で、`En > 0` のイベントだけROOTへ保存します。

`En` はTrack ID 1がシンチレータ内にいるときのvertex kinetic energyです。

したがって、先輩コードのROOT treeは、最初から特定のイベントだけを含む可能性があります。

### 現行コード

`src/EventAction.cc:60` 以降で、全イベントについて1行を確定します。

### どちらが優れているか

一般的には現行コードの方が優れています。

保存時にイベントを削除すると、後から選別条件を変更できません。全イベントを保存しておけば、解析時に複数の条件を比較できます。

### 推奨実装

全イベントを保存し、次のようなフラグまたは値を一緒に保存します。

- `PrimaryEnteredScinti`
- `PrimaryEntryEnergy`
- `HasScintiEdep`
- `HasGeneratedPhoton`
- `HasPMTArrival`
- `HasPMTDetection`

解析時にこれらを使って母集団を定義します。

先輩コードと現行コードのROOTを比較する場合、現行側へ単純に `Scinti_Edep > 0` を適用しても、旧コードの `En > 0` と完全には一致しません。

## 10. データの所有とイベント集計

### 先輩コード

`Scinti_E`、`Scinti_PMT_E` などをグローバルvectorとして定義し、SD、SteppingAction、RunActionから共有しています。

この方式には次の問題があります。

- どのクラスが値を所有するか分かりにくい。
- 初期化と保存の順序が複数クラスへ分散する。
- columnと変数の対応が固定番号に依存する。
- 通常のグローバル変数はマルチスレッド実行で共有状態になり得る。

### 現行コード

- `ScintiSD` がシンチレータのイベント集計値を持つ。
- `CathodeSD` がPMT別光子数を持つ。
- `EventAction` が光子ごとのvectorとイベント要約を持つ。
- `AnalysisOutput` がROOT column IDと書き込みを担当する。

### どちらが優れているか

現行コードの方が設計として優れています。

物理量の生成、イベント集計、ROOT出力の責務が分離されており、値の寿命と所有者を追いやすいためです。

### 推奨実装

現在の責務分離を基本とし、単純なイベントスカラーだけなら現行のイベントメンバ方式でも十分です。次を守ります。

- `AddNtupleRow()` はイベントごとに1か所だけで呼ぶ。
- SDは測定値の作成とイベント内集計へ集中する。
- `EventAction` は複数SDにまたがる派生量を計算する。
- `AnalysisOutput` はcolumn作成とFillを担当する。
- column IDをソース中の固定整数として分散させない。

一方、光子ごとの時刻・位置・エネルギーなど、詳細なhitを複数保存する場合は、Geant4標準の次の構造がより推奨されます。

```text
Sensitive Detector
    ↓ G4VHitを生成
G4THitsCollection
    ↓ collection IDで取得
EventAction
    ↓ イベント集計
AnalysisOutput
```

この構造には次の利点があります。

- hitに属する時刻・x・y・z・光子エネルギーを1オブジェクトへまとめられる。
- 現行のように時刻、x、y、zを別々のvectorで管理したときの要素ずれを防げる。
- SDとROOT出力の直接的な依存を弱められる。
- Geant4のイベントごとのcollection管理と整合する。

`extended/optical/LXe` と `extended/optical/wls` は、PMTまたは光子hitを `G4THitsCollection` で管理しています。

## 11. 物理パラメータの違い

取得ロジックが同じでも、材料パラメータが異なれば結果は一致しません。

| パラメータ | 先輩コード | 現行コード | 影響 |
|---|---:|---:|---|
| `SCINTILLATIONYIELD` | `8000 / MeV` | `10000 / MeV` | 現行の公称yieldは25%高い |
| Birks定数 | `0.111 mm/MeV` | 約`0.1269 mm/MeV` | 現行の方が消光を強く評価する方向 |
| PMT `REFLECTIVITY` | 0 | 0 | 一致 |
| PMT `EFFICIENCY` | 1 | 1 | 一致、理想的な100%検出面 |

参照箇所:

- 先輩コードyield: `src/Geometry.cc:370`
- 先輩コードBirks定数: `include/myParameter.hh:14`
- 現行yield: `src/Material/BC408Mat.cc:60`
- 現行Birks定数: `src/Material/BC408Mat.cc:70`
- 現行Cathode surface: `src/Surface/UROKO/CathodeSurface.cc:13`

### どちらが優れているか

数値が大きい方、新しい方が優れているとは限りません。

物理パラメータは、次の優先順位で決めるべきです。

1. 実際に使用する材料のメーカー値
2. 実測値
3. 信頼できる論文値
4. 実験データとの校正

コード上の推奨点としては、現行コードのように単位を含む式でBirks定数を記述し、材料クラスへ設定を集約する方が、値の由来と次元を確認しやすくなります。

## 12. 現行コード固有の追加注意点

### 5000 stepを超えたtrackの停止

`src/ScintiSD.cc:37` のコメントでは「閉じ込められた光子」を止める処理となっていますが、実装にはoptical photon判定がありません。

```cpp
if (aStep->GetTrack()->GetCurrentStepNumber() > 5000) {
    aStep->GetTrack()->SetTrackStatus(fStopAndKill);
    return false;
}
```

この条件は、シンチレータ内で5000 stepを超えた全粒子を停止します。

#### どちらが優れているか

物理追跡を不必要に打ち切らないという点では、この処理を持たない先輩コードの方が安全です。ただし、光子トラップにより計算が終了しない問題への対策自体は必要になる場合があります。

#### 推奨実装

- 少なくともoptical photonだけへ限定する。
- step数上限を物理カットと混同しない。
- トラップの原因がgeometry overlap、表面法線、微小step、完全反射設定にないか先に調べる。
- 上限で停止した光子数を別カウンタへ記録し、結果への影響を評価できるようにする。

### 無ヒット時の半径

無ヒット時、local位置は次のsentinel値です。

```text
(-99999, -99999, -99999)
```

しかし `src/EventAction.cc:57` では常に次を計算します。

```cpp
sqrt(posL.x() * posL.x() + posL.y() * posL.y())
```

その結果、無ヒットイベントの半径は約 `141420 mm` になります。

#### どちらが優れているか

先輩コードは無ヒットイベントを保存しないことが多いため、このsentinel計算問題は表面化しません。しかし、イベントを捨てる方法も推奨されません。したがって、どちらも理想的ではありません。

#### 推奨実装

- `HasScintiEdep` のような有効性フラグを保存する。
- 無ヒット時は半径計算を行わず、sentinel、`NaN`、または別の明示値を保存する。
- 解析では値の大きさではなく、有効性フラグで選別する。

## 13. `Scinti_direct` の問題

先輩コードでは、最初のエネルギー減少について次の独自分類を行っています。

```text
1 = direct
2 = elastic
3 = inelastic / others
```

しかし外側で `TrackID == 1` に限定し、`neutron_ID` も1へ設定しているため、`TrackID != neutron_ID` による分類3は到達不能です。

また、elastic/inelasticの分類はprocess名ではなく、pre-step kinetic energyとvertex kinetic energyが完全一致するかで決めています。

### どちらが優れているか

現行コードはこの曖昧な分類を出力していないため、誤解を生まない点では現行の方が安全です。ただし、反応分類の情報そのものが不要という意味ではありません。

### 推奨実装

反応を分類したい場合は、次を確認します。

```cpp
const auto* process =
    step->GetPostStepPoint()->GetProcessDefinedStep();
```

その上で、例えば次のprocess名を区別します。

- `hadElastic`
- `neutronInelastic`
- `nCapture`

エネルギーが以前より減っているかではなく、そのstepを定義した物理processで分類する方が推奨されます。

## 先輩コードと現行コードを公平に比較する手順

### 1. 先輩が実際に使用したソースを特定する

特に次を確認します。

- `Geometry.cc` と `Geometry_org.cc` のどちらを使ったか。
- PMT logical volumeとCathode logical volumeのどちらへSDを付与したか。
- `SensitiveDetectors.cc` と `SensitiveDetectors.cc.AllPhotonTime` のどちらを使ったか。
- Geant4のバージョンと使用したphysics list。

### 2. 材料・surface設定を揃える

- `SCINTILLATIONYIELD`
- Birks定数
- `ABSLENGTH`
- `RINDEX`
- 発光スペクトル
- PMT `EFFICIENCY`
- PMT `REFLECTIVITY`

### 3. イベント母集団を揃える

先輩コードの `En > 0` と同じ条件を現行側でも再現できるよう、primary入射情報を確認します。

### 4. 同じ定義の量だけを比較する

| 先輩コードbranch | 現行branch | 比較方法 |
|---|---|---|
| `EDep` | `Scinti_Edep` | 条件を揃えれば比較可能 |
| `Ele_Eq` | `Scinti_Evis` | Birks定数と対象stepを揃えた後に比較 |
| `photon` | `Scinti_Photons` | 旧は全二次粒子なので直接比較しない |
| `PMT_hits` | `PMT1_Photons` | 旧GeometryのSD付与先を確認してから比較 |
| `PMT2_hits` | `PMT2_Photons` | 同上 |
| `PMT_T` | `min(PMT1_HitTimes)` | 現行vectorの最小値と比較 |
| `PMT2_T` | `min(PMT2_HitTimes)` | 現行vectorの最小値と比較 |
| `nHit_Pos` | `Scinti_HitPos_Global` | first hitの定義が違うため直接比較しない |

### 5. PMT hitが本当に記録されているか確認する

先輩コードのROOTで `PMT_hits` と `PMT2_hits` がすべて0なら、物理的に光が届かなかった可能性だけでなく、SD付与先の不整合も疑う必要があります。

## 今後の実装で推奨する物理量の名前

曖昧な `photon`、`hit`、`efficiency` を避け、定義を名前へ含めると比較しやすくなります。

```text
Scinti_TotalEdep
Scinti_VisibleEdep
Scinti_EdepStepCount
Scinti_FirstEdepTime
Scinti_FirstEdepPosGlobal
Scinti_FirstEdepPosLocal
Scinti_ScintillationPhotonCount
Scinti_CerenkovPhotonCount

PMT1_ArrivedPhotonCount
PMT1_DetectedPhotonCount
PMT1_DetectedPhotonTimes
PMT1_DetectedPhotonPosLocal

PMT2_ArrivedPhotonCount
PMT2_DetectedPhotonCount
PMT2_DetectedPhotonTimes
PMT2_DetectedPhotonPosLocal

PMT_CollectionEfficiency
PMT_DetectionEfficiency
```

## 参考になるGeant4 example

### `extended/optical/LXe`

```text
/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/LXe
```

- `src/LXeScintSD.cc:70`
  - `GetTotalEnergyDeposit()` が0のstepを除外しています。
  - pre/post位置の中点をhit位置として保存しています。
- `src/LXeStackingAction.cc:47`
  - creator processを使い、Scintillation光とCerenkov光を別々に数えています。
- `src/LXeSteppingAction.cc:148`
  - optical boundary statusを確認しています。
  - `Detection` の場合だけPMT SDを手動で起動しています。
- `src/LXePMTSD.cc:104`
  - post-step touchableからPMT番号を取得し、PMT別に光子数をまとめています。

### `extended/optical/wls`

```text
/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/wls
```

- `wls.cc:68`
  - `G4OpticalParameters::SetBoundaryInvokeSD(true)` を設定しています。
- `src/WLSPhotonDetSD.cc:69`
  - optical photonだけを受理します。
  - 到達時刻、位置、光子エネルギーを保存します。
  - 到達位置を検出器local座標へ変換します。
- `src/WLSSteppingAction.cc:318`
  - `Detection` の場合、boundary processによってSDが呼ばれたものとして扱います。

### `extended/optical/OpNovice2`

```text
/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/OpNovice2
```

- `src/SteppingAction.cc:141`
  - `fGeomBoundary` でのみboundary statusを評価しています。
- `src/SteppingAction.cc:228`
  - `Detection` を他の反射・吸収statusと分けて集計しています。
- `src/SteppingAction.cc:413`
  - 二次光子のcreator process、生成エネルギー、生成時刻を確認しています。

## 実装前に決めるチェックリスト

新しい物理量を追加する前に、次を明文化します。

1. 何を数えるのか。
   - step
   - track
   - physical interaction
   - generated photon
   - arrived photon
   - detected photon
2. どの粒子を対象にするのか。
3. どのvolumeで生成された量か。
4. pre-step、post-step、中点のどこを位置とするのか。
5. 「最初」は処理順か、最小global timeか。
6. 単位は何か。
7. イベントを保存時に除外するか、解析時に選別するか。
8. 効率の分子と分母で光子の起源が一致しているか。
9. PMT到達と `Detection` を区別しているか。
10. geometryのcopy number変更に依存しすぎていないか。

## まとめ

全体として、現行の `ScintiSD`・`CathodeSD`・`EventAction`・`AnalysisOutput` に役割を分けた構成は、先輩コードのグローバルvectorへ直接集計する構成より推奨されます。

特に改善されている点は次です。

- エネルギー付与と可視エネルギーを同じSD・同じstepで取得している。
- 全二次粒子ではなくoptical photonを選んで数えている。
- PMTのSD付与先と境界判定が整合している。
- PMT光子の全時刻と位置を保存している。
- 全イベントを保存し、物理量の作成とROOT出力の責務を分離している。

一方、現行コードでも次は改善候補です。

- Scintillation光とCerenkov光をcreator processで分離する。
- PMT到達数と `Detection` 数を分ける。
- `PMT_SUM_Efficiency` を合計または平均のどちらか明確な名前と式に揃える。
- first hitを処理順ではなく、目的に応じた明確な条件で定義する。
- `Scinti_Hits` を実際の意味に合う名前へ変更する。
- 無ヒット時の半径計算を有効性フラグで保護する。
- 光子停止条件をoptical photonへ限定し、停止数を記録する。
- 光子ごとの詳細情報が増える場合は、時刻・位置の並列vectorから `G4THitsCollection` へ移行する。

最も重要な考え方は、**同じ「光子数」「hit数」「効率」という名前でも、生成・到達・検出のどの段階かによって意味が異なる**という点です。物理量の名前、取得条件、単位、イベント選別条件をセットで定義することで、異なるコードや異なる計算条件を正しく比較できるようになります。
