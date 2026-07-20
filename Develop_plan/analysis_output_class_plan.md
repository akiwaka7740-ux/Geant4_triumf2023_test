# AnalysisOutput 専用クラス導入 実装計画

## 目的

`G4AnalysisManager` の column 作成、column ID 管理、`FillNtuple...Column()` 呼び出しを `AnalysisOutput` クラスへ集約する。

現在は以下のように責務が分散している。

```text
RunAction.cc   : ntuple column を作成する
ScintiSD.cc    : Scinti 系 column に Fill する
CathodeSD.cc   : PMT photon count column に Fill する
EventAction.cc : radius / efficiency column に Fill する
```

このままだと、物理量を追加するたびに複数ファイルで column ID を追いかける必要がある。

`AnalysisOutput` を導入し、各クラスからは意味のある関数を呼ぶだけにする。

```cpp
analysisOutput->FillScinti(...);
analysisOutput->FillPMTPhotons(...);
analysisOutput->FillEventSummary(...);
```

## 到達目標

最終的には、以下の状態を目指す。

```text
RunAction
  - ROOT ファイルの open/write/close を担当する
  - AnalysisOutput に ntuple column 作成を依頼する

AnalysisOutput
  - ntuple column 作成
  - column ID の保存
  - FillNtuple...Column() 呼び出し
  - AddNtupleRow() 呼び出し

ScintiSD
  - シンチレータ内の測定値を集める
  - AnalysisOutput::FillScinti() を呼ぶ

CathodeSD
  - PMT 到達 photon を数える
  - AnalysisOutput::FillPMTPhotons() を呼ぶ

EventAction
  - event 単位の集計を行う
  - AnalysisOutput::FillEventSummary() を呼ぶ
  - event 開始時に vector を clear する
```

## 新規追加するファイル

候補:

```text
include/AnalysisOutput.hh
src/AnalysisOutput.cc
```

## AnalysisOutput の基本設計

### ヘッダのイメージ

```cpp
#ifndef ANALYSISOUTPUT_HH
#define ANALYSISOUTPUT_HH

#include "globals.hh"

class EventAction;

class AnalysisOutput {
public:
    AnalysisOutput() = default;
    ~AnalysisOutput() = default;

    void Book(EventAction* eventAction);

    void FillScinti(
        G4double edep,
        G4double generatedPhotons,
        G4int hitCount
    );

    void FillPMTPhotons(
        G4int pmt1Photons,
        G4int pmt2Photons
    );

    void FillEventSummary(
        G4double scintiHitRadius,
        G4double pmt1Efficiency,
        G4double pmt2Efficiency
    );

    void AddRow();

private:
    G4int fScintiEdep = -1;
    G4int fScintiPhotons = -1;
    G4int fScintiHits = -1;
    G4int fScintiHitPosGlobal = -1;
    G4int fScintiHitPosLocal = -1;
    G4int fScintiHitPosRadius = -1;

    G4int fPMTSumPhotons = -1;
    G4int fPMTSumEfficiency = -1;
    G4int fPMT1Photons = -1;
    G4int fPMT1Efficiency = -1;
    G4int fPMT2Photons = -1;
    G4int fPMT2Efficiency = -1;

    G4int fPMT1HitTimes = -1;
    G4int fPMT1HitPosX = -1;
    G4int fPMT1HitPosY = -1;
    G4int fPMT1HitPosZ = -1;

    G4int fPMT2HitTimes = -1;
    G4int fPMT2HitPosX = -1;
    G4int fPMT2HitPosY = -1;
    G4int fPMT2HitPosZ = -1;
};

#endif
```

## Book() の役割

`Book(EventAction* eventAction)` では、現在 `RunAction.cc` にある ntuple 作成処理を移す。

### 処理内容

1. `G4AnalysisManager::Instance()` を取得する。
2. `SetNtupleMerging(true)` を設定する。
3. `CreateNtuple("tree", "tree")` を呼ぶ。
4. 全 column を作成し、戻り値をメンバ変数に保存する。
5. vector column には `EventAction` が持つ vector 参照を渡す。
6. `FinishNtuple(0)` を呼ぶ。

### 実装イメージ

```cpp
void AnalysisOutput::Book(EventAction* eventAction)
{
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetNtupleMerging(true);
    analysisManager->CreateNtuple("tree", "tree");

    fScintiEdep =
        analysisManager->CreateNtupleDColumn("Scinti_Edep");
    fScintiPhotons =
        analysisManager->CreateNtupleDColumn("Scinti_Photons");
    fScintiHits =
        analysisManager->CreateNtupleIColumn("Scinti_Hits");

    fScintiHitPosGlobal =
        analysisManager->CreateNtupleDColumn(
            "Scinti_HitPos_Global",
            eventAction->GetScintiPosGlobalRef()
        );
    fScintiHitPosLocal =
        analysisManager->CreateNtupleDColumn(
            "Scinti_HitPos_Local",
            eventAction->GetScintiPosLocalRef()
        );
    fScintiHitPosRadius =
        analysisManager->CreateNtupleDColumn("Scinti_HitPos_Radius");

    fPMTSumPhotons =
        analysisManager->CreateNtupleIColumn("PMT_SUM_Photons");
    fPMTSumEfficiency =
        analysisManager->CreateNtupleDColumn("PMT_SUM_Efficiency");
    fPMT1Photons =
        analysisManager->CreateNtupleIColumn("PMT1_Photons");
    fPMT1Efficiency =
        analysisManager->CreateNtupleDColumn("PMT1_Efficiency");
    fPMT2Photons =
        analysisManager->CreateNtupleIColumn("PMT2_Photons");
    fPMT2Efficiency =
        analysisManager->CreateNtupleDColumn("PMT2_Efficiency");

    fPMT1HitTimes =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitTimes",
            eventAction->GetHitTimeListRef(0)
        );
    fPMT1HitPosX =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitPos_X",
            eventAction->GetHitPosXListRef(0)
        );
    fPMT1HitPosY =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitPos_Y",
            eventAction->GetHitPosYListRef(0)
        );
    fPMT1HitPosZ =
        analysisManager->CreateNtupleDColumn(
            "PMT1_HitPos_Z",
            eventAction->GetHitPosZListRef(0)
        );

    fPMT2HitTimes =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitTimes",
            eventAction->GetHitTimeListRef(1)
        );
    fPMT2HitPosX =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitPos_X",
            eventAction->GetHitPosXListRef(1)
        );
    fPMT2HitPosY =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitPos_Y",
            eventAction->GetHitPosYListRef(1)
        );
    fPMT2HitPosZ =
        analysisManager->CreateNtupleDColumn(
            "PMT2_HitPos_Z",
            eventAction->GetHitPosZListRef(1)
        );

    analysisManager->FinishNtuple(0);
}
```

## Fill 関数の設計

### FillScinti()

現在 `ScintiSD::EndOfEvent()` で行っている Fill を移す。

```cpp
void AnalysisOutput::FillScinti(
    G4double edep,
    G4double generatedPhotons,
    G4int hitCount
) {
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleDColumn(fScintiEdep, edep);
    analysisManager->FillNtupleDColumn(fScintiPhotons, generatedPhotons);
    analysisManager->FillNtupleIColumn(fScintiHits, hitCount);
}
```

### FillPMTPhotons()

現在 `CathodeSD::EndOfEvent()` で行っている Fill を移す。

```cpp
void AnalysisOutput::FillPMTPhotons(
    G4int pmt1Photons,
    G4int pmt2Photons
) {
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleIColumn(
        fPMTSumPhotons,
        pmt1Photons + pmt2Photons
    );
    analysisManager->FillNtupleIColumn(fPMT1Photons, pmt1Photons);
    analysisManager->FillNtupleIColumn(fPMT2Photons, pmt2Photons);
}
```

### FillEventSummary()

現在 `EventAction::EndOfEventAction()` で行っている radius / efficiency の Fill を移す。

```cpp
void AnalysisOutput::FillEventSummary(
    G4double scintiHitRadius,
    G4double pmt1Efficiency,
    G4double pmt2Efficiency
) {
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleDColumn(
        fScintiHitPosRadius,
        scintiHitRadius
    );
    analysisManager->FillNtupleDColumn(
        fPMTSumEfficiency,
        pmt1Efficiency + pmt2Efficiency
    );
    analysisManager->FillNtupleDColumn(fPMT1Efficiency, pmt1Efficiency);
    analysisManager->FillNtupleDColumn(fPMT2Efficiency, pmt2Efficiency);
}
```

### AddRow()

`EventAction::EndOfEventAction()` で最後に呼ぶ。

```cpp
void AnalysisOutput::AddRow()
{
    G4AnalysisManager::Instance()->AddNtupleRow(0);
}
```

## AnalysisOutput の受け渡し方

### 推奨案: EventAction に AnalysisOutput pointer を持たせる

`ScintiSD` や `CathodeSD` は、すでに `G4EventManager` から `EventAction` を取得できる。

そのため、`EventAction` に `AnalysisOutput*` を持たせ、SD 側は `EventAction` 経由で `AnalysisOutput` を使う。

### EventAction.hh の修正イメージ

```cpp
class AnalysisOutput;
```

```cpp
public:
    void SetAnalysisOutput(AnalysisOutput* output) {
        fAnalysisOutput = output;
    }

    AnalysisOutput* GetAnalysisOutput() const {
        return fAnalysisOutput;
    }

private:
    AnalysisOutput* fAnalysisOutput = nullptr;
```

## RunAction 側の変更方針

`RunAction` が `AnalysisOutput` を所有する案が自然。

### RunAction.hh の修正イメージ

```cpp
#include "AnalysisOutput.hh"
```

```cpp
private:
    EventAction* fEventAction;
    AnalysisOutput fAnalysisOutput;
```

### RunAction.cc の修正イメージ

コンストラクタで以下を行う。

```cpp
RunAction::RunAction(EventAction* eventAction)
 : fEventAction(eventAction)
{
    fAnalysisOutput.Book(fEventAction);
    fEventAction->SetAnalysisOutput(&fAnalysisOutput);
}
```

これにより、`EventAction`、`ScintiSD`、`CathodeSD` は `EventAction` 経由で同じ `AnalysisOutput` にアクセスできる。

## ScintiSD 側の変更方針

`ScintiSD::EndOfEvent()` では、`analysisManager` を直接使わない。

修正イメージ:

```cpp
void ScintiSD::EndOfEvent(G4HCofThisEvent*)
{
    auto eventAction =
        static_cast<EventAction*>(
            G4EventManager::GetEventManager()->GetUserEventAction()
        );

    if (!eventAction) return;

    auto output = eventAction->GetAnalysisOutput();
    if (!output) return;

    output->FillScinti(fTotalEdep, fGeneratedPhotons, fHitCount);
}
```

## CathodeSD 側の変更方針

`CathodeSD::EndOfEvent()` でも、`analysisManager` を直接使わない。

修正イメージ:

```cpp
void CathodeSD::EndOfEvent(G4HCofThisEvent*)
{
    auto eventAction =
        static_cast<EventAction*>(
            G4EventManager::GetEventManager()->GetUserEventAction()
        );

    if (!eventAction) return;

    auto output = eventAction->GetAnalysisOutput();
    if (!output) return;

    output->FillPMTPhotons(fPhotonCount[0], fPhotonCount[1]);
}
```

## EventAction 側の変更方針

`EventAction::EndOfEventAction()` では、efficiency と radius を計算した後、`AnalysisOutput` に渡す。

修正イメージ:

```cpp
if (fAnalysisOutput) {
    fAnalysisOutput->FillEventSummary(fRadius, fEff[0], fEff[1]);
    fAnalysisOutput->AddRow();
}
```

この方式なら、`EventAction` も column ID を知らなくてよい。

## 実装手順

### Step 1: AnalysisOutput.hh / AnalysisOutput.cc を追加

`Book()`、`FillScinti()`、`FillPMTPhotons()`、`FillEventSummary()`、`AddRow()` を定義する。

### Step 2: RunAction に AnalysisOutput を持たせる

`RunAction` のコンストラクタで `fAnalysisOutput.Book(fEventAction)` を呼ぶ。

その後、`fEventAction->SetAnalysisOutput(&fAnalysisOutput)` を呼ぶ。

### Step 3: EventAction に AnalysisOutput pointer を追加

`SetAnalysisOutput()` と `GetAnalysisOutput()` を追加する。

`EndOfEventAction()` の固定 ID Fill を `fAnalysisOutput->FillEventSummary()` に置き換える。

`AddNtupleRow(0)` も `fAnalysisOutput->AddRow()` に置き換える。

### Step 4: ScintiSD の Fill を AnalysisOutput に移す

`ScintiSD::EndOfEvent()` から直接の `FillNtuple...Column()` をなくす。

### Step 5: CathodeSD の Fill を AnalysisOutput に移す

`CathodeSD::EndOfEvent()` から直接の `FillNtuple...Column()` をなくす。

photon count は必ず `FillNtupleIColumn()` で扱う。

### Step 6: RunAction から column 作成処理を削除する

`RunAction.cc` には ROOT file の open/write/close と、`AnalysisOutput::Book()` 呼び出しだけを残す。

### Step 7: include 関係を整理する

循環 include を避けるため、可能な場所では前方宣言を使う。

特に `EventAction.hh` では、

```cpp
class AnalysisOutput;
```

を使い、`AnalysisOutput.hh` の include は `.cc` 側に寄せるのが望ましい。

## 注意点

### 1. AnalysisOutput の寿命

`EventAction` が `AnalysisOutput*` を持つため、`AnalysisOutput` は `EventAction` より短命になってはいけない。

`RunAction` が `AnalysisOutput` をメンバとして持つ場合、現在の `ActionInitialization` の構成では基本的に問題ない見込み。

ただし、master thread と worker thread の両方で `RunAction` / `EventAction` が作られるため、マルチスレッド時の挙動は確認する必要がある。

### 2. master thread 側

`BuildForMaster()` でも `EventAction` と `RunAction` が作られている。

master thread で SD の `EndOfEvent()` は動かないはずだが、`RunAction` の ntuple booking と merging との関係は確認が必要。

### 3. vector column は Fill しない

`PMT1_HitTimes` や `PMT1_HitPos_X` のような vector column は、作成時に vector 参照を渡す。

そのため、イベント中は `EventAction::AddHitTime()` や `AddHitPos()` で vector に値を入れ、最後に `AddRow()` すればよい。

### 4. AddRow は 1 event につき 1 回

`AddRow()` は `EventAction::EndOfEventAction()` の最後で 1 回だけ呼ぶ。

`ScintiSD` や `CathodeSD` では呼ばない。

### 5. Fill の順番

1 event 内であれば、scalar column の Fill 順は基本的に column ID が正しければ問題ない。

ただし、読みやすさのために以下の順番を推奨する。

```text
ScintiSD::EndOfEvent()
CathodeSD::EndOfEvent()
EventAction::EndOfEventAction()
  - efficiency 計算
  - FillEventSummary()
  - AddRow()
```

## メリット

- column ID の手書きがなくなる。
- 物理量を追加するとき、基本的に `AnalysisOutput` だけを見ればよくなる。
- `ScintiSD` や `CathodeSD` の責務が「測定値を作ること」に集中する。
- `RunAction` の責務が「run と file の管理」に集中する。
- 出力仕様を 1 ファイルで把握できる。

## デメリット

- `AnalysisOutput` という新しいクラスが増える。
- 最初の移行では複数ファイルを触る必要がある。
- `EventAction` 経由で `AnalysisOutput` にアクセスするため、ポインタの寿命に注意が必要。
- 小規模な出力だけなら `AnalysisColumnIDs` 方式より少し大きな変更になる。

## AnalysisColumnIDs 方式との比較

### AnalysisColumnIDs 方式

- 変更量が少ない。
- 既存構造を保ちやすい。
- ただし Fill 処理は各ファイルに残る。

### AnalysisOutput 方式

- 変更量はやや多い。
- column ID と Fill 処理を 1 箇所に集約できる。
- 将来の物理量追加には強い。

## 推奨判断

短期的に ID ずれだけを防ぐなら `AnalysisColumnIDs` 方式で十分。

今後、PMT、Scinti、検出器 ID、粒子種、hit position、time profile など、保存量が増える見込みがあるなら `AnalysisOutput` 方式がよい。

特に、複数検出器を同時に有効化する予定があるなら、早めに `AnalysisOutput` へ集約しておく価値がある。

## 実装後の確認項目

1. `RunAction.cc` に `CreateNtuple...Column()` の長い列定義が残っていない。
2. `ScintiSD.cc` に `FillNtuple...Column()` が残っていない。
3. `CathodeSD.cc` に `FillNtuple...Column()` が残っていない。
4. `EventAction.cc` に固定 ID の `FillNtuple...Column(5, ...)` などが残っていない。
5. `AnalysisOutput.cc` にすべての column 作成と Fill 関数が集約されている。
6. photon count は `IColumn` / `FillNtupleIColumn()` になっている。
7. efficiency や radius は `DColumn` / `FillNtupleDColumn()` になっている。
8. vector column は `CreateNtupleDColumn(name, vectorRef)` だけで扱われている。
9. `AddRow()` が 1 event につき 1 回だけ呼ばれている。

## 参考 example

Geant4 11.2.2 の TestEm9 は、完全に同じ設計ではないが、column ID を保存して Fill 時に使う考え方が参考になる。

```text
/home/hashizume/Geant4/geant4-11.2.2/examples/extended/electromagnetic/TestEm9
```

参考ファイル:

```text
examples/extended/electromagnetic/TestEm9/include/Histo.hh
examples/extended/electromagnetic/TestEm9/src/Histo.cc
```

TestEm9 では `CreateNtupleIColumn()` や `CreateNtupleDColumn()` の戻り値を `fTupleI`、`fTupleD` に保存し、Fill 時にその ID を使っている。

`AnalysisOutput` 方式は、この考え方を現在のプロジェクト用にもう少し明示的なクラスへ分離する案である。
