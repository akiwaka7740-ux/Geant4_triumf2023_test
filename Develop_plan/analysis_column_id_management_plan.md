# analysisManager column ID 管理改善 実装計画

## 目的

物理量や ntuple column を追加するたびに、`FillNtuple...Column(6, value)` のような手書き ID を修正しなくてよい設計にする。

現在は `RunAction.cc` で column を作成し、`ScintiSD.cc`、`CathodeSD.cc`、`EventAction.cc` で ID 番号を直接指定して値を詰めている。そのため、途中に column を追加すると ID がずれやすい。

この計画では、Geant4 の `CreateNtuple...Column()` が返す column ID を保存し、その保存済み ID を Fill 側で使う方式へ移行する。

## 基本方針

1. column ID を数字で直接書かない。
2. `CreateNtuple...Column()` の戻り値を必ず保存する。
3. 保存した ID を 1 箇所に集約する。
4. `ScintiSD`、`CathodeSD`、`EventAction` は、その ID を参照して Fill する。
5. 将来的には Fill 処理自体も専用クラスに集約できる形にする。

## 推奨する設計

まずは大きな設計変更を避け、`AnalysisColumnIDs` という ID 保持用の構造体を導入する。

候補ファイル:

```text
include/AnalysisColumnIDs.hh
```

中身のイメージ:

```cpp
#ifndef ANALYSISCOLUMNIDS_HH
#define ANALYSISCOLUMNIDS_HH

#include "globals.hh"

struct AnalysisColumnIDs {
    G4int Scinti_Edep = -1;
    G4int Scinti_Photons = -1;
    G4int Scinti_Hits = -1;
    G4int Scinti_HitPos_Global = -1;
    G4int Scinti_HitPos_Local = -1;
    G4int Scinti_HitPos_Radius = -1;

    G4int PMT_SUM_Photons = -1;
    G4int PMT_SUM_Efficiency = -1;
    G4int PMT1_Photons = -1;
    G4int PMT1_Efficiency = -1;
    G4int PMT2_Photons = -1;
    G4int PMT2_Efficiency = -1;

    G4int PMT1_HitTimes = -1;
    G4int PMT1_HitPos_X = -1;
    G4int PMT1_HitPos_Y = -1;
    G4int PMT1_HitPos_Z = -1;

    G4int PMT2_HitTimes = -1;
    G4int PMT2_HitPos_X = -1;
    G4int PMT2_HitPos_Y = -1;
    G4int PMT2_HitPos_Z = -1;
};

#endif
```

## ID の所有場所

`AnalysisColumnIDs` のインスタンスをどこに置くかは重要。

現実的には、まず `RunAction` が所有する案が扱いやすい。

理由:

- column を作成しているのが `RunAction`。
- `RunAction` のコンストラクタで ID を確定できる。
- `EventAction` はすでに `RunAction` に渡されているため、ID を渡す導線を作りやすい。

ただし、`ScintiSD` や `CathodeSD` は `RunAction` を直接知らない。したがって、ID を参照する方法を別途考える必要がある。

## 実装案 A: EventAction に ID を持たせる

最初の改善としては、この案が比較的わかりやすい。

### 概要

- `RunAction` が column を作る。
- `RunAction` が戻り値を `AnalysisColumnIDs` に保存する。
- その ID 情報を `EventAction` に渡す。
- `EventAction` 経由で ID を取得する。

### 変更対象

```text
include/AnalysisColumnIDs.hh
include/EventAction.hh
src/EventAction.cc
include/RunAction.hh
src/RunAction.cc
src/ScintiSD.cc
src/CathodeSD.cc
```

### EventAction 側の修正案

`EventAction.hh` に `AnalysisColumnIDs` を保持するメンバを追加する。

```cpp
#include "AnalysisColumnIDs.hh"
```

```cpp
public:
    void SetColumnIDs(const AnalysisColumnIDs* ids) { fColumnIDs = ids; }
    const AnalysisColumnIDs* GetColumnIDs() const { return fColumnIDs; }

private:
    const AnalysisColumnIDs* fColumnIDs = nullptr;
```

`const AnalysisColumnIDs*` にする理由は、`EventAction` 側では ID を変更しないため。

### RunAction 側の修正案

`RunAction.hh` に ID 保持メンバを追加する。

```cpp
#include "AnalysisColumnIDs.hh"
```

```cpp
private:
    EventAction* fEventAction;
    AnalysisColumnIDs fColumnIDs;
```

`RunAction.cc` では column 作成時に戻り値を保存する。

```cpp
fColumnIDs.Scinti_Edep =
    analysisManager->CreateNtupleDColumn("Scinti_Edep");

fColumnIDs.Scinti_Photons =
    analysisManager->CreateNtupleDColumn("Scinti_Photons");

fColumnIDs.Scinti_Hits =
    analysisManager->CreateNtupleIColumn("Scinti_Hits");
```

全 column を同じ形式で保存する。

最後に、`RunAction` コンストラクタ内で `EventAction` に ID を渡す。

```cpp
fEventAction->SetColumnIDs(&fColumnIDs);
```

注意点として、`RunAction` と `EventAction` の寿命関係を確認する必要がある。現在の構成では `ActionInitialization::Build()` 内で `EventAction` を作り、そのポインタを `RunAction` に渡しているため、基本的には問題ない見込み。

## ScintiSD の修正方針

現在の `ScintiSD.cc` では以下のように固定 ID を使っている。

```cpp
analysisManager->FillNtupleDColumn(0, fTotalEdep);
analysisManager->FillNtupleDColumn(1, fGeneratedPhotons);
analysisManager->FillNtupleIColumn(2, fHitCount);
```

修正後は、現在の event action から column ID を取得して使う。

イメージ:

```cpp
auto eventAction =
    static_cast<EventAction*>(
        G4EventManager::GetEventManager()->GetUserEventAction()
    );

auto ids = eventAction->GetColumnIDs();
if (!ids) return;

analysisManager->FillNtupleDColumn(ids->Scinti_Edep, fTotalEdep);
analysisManager->FillNtupleDColumn(ids->Scinti_Photons, fGeneratedPhotons);
analysisManager->FillNtupleIColumn(ids->Scinti_Hits, fHitCount);
```

この変更により、`Scinti_Edep` の column 位置が変わっても `ScintiSD.cc` を修正しなくてよくなる。

## CathodeSD の修正方針

現在の `CathodeSD.cc` では以下のように固定 ID を使っている。

```cpp
analysisManager->FillNtupleDColumn(6, fPhotonCount[0] + fPhotonCount[1]);
analysisManager->FillNtupleDColumn(8, fPhotonCount[0]);
analysisManager->FillNtupleDColumn(10, fPhotonCount[1]);
```

問題点は 2 つある。

1. ID が固定値。
2. `CreateNtupleIColumn` で作った photon count に対して `FillNtupleDColumn` を使っている。

修正後のイメージ:

```cpp
auto eventAction =
    static_cast<EventAction*>(
        G4EventManager::GetEventManager()->GetUserEventAction()
    );

auto ids = eventAction->GetColumnIDs();
if (!ids) return;

analysisManager->FillNtupleIColumn(
    ids->PMT_SUM_Photons,
    fPhotonCount[0] + fPhotonCount[1]
);
analysisManager->FillNtupleIColumn(ids->PMT1_Photons, fPhotonCount[0]);
analysisManager->FillNtupleIColumn(ids->PMT2_Photons, fPhotonCount[1]);
```

## EventAction の修正方針

現在の `EventAction.cc` では以下のように固定 ID を使っている。

```cpp
analysisManager->FillNtupleDColumn(5, fRadius);
analysisManager->FillNtupleDColumn(7, fEff[0] + fEff[1]);
analysisManager->FillNtupleDColumn(9, fEff[0]);
analysisManager->FillNtupleDColumn(11, fEff[1]);
```

修正後のイメージ:

```cpp
if (!fColumnIDs) return;

analysisManager->FillNtupleDColumn(fColumnIDs->Scinti_HitPos_Radius, fRadius);
analysisManager->FillNtupleDColumn(fColumnIDs->PMT_SUM_Efficiency, fEff[0] + fEff[1]);
analysisManager->FillNtupleDColumn(fColumnIDs->PMT1_Efficiency, fEff[0]);
analysisManager->FillNtupleDColumn(fColumnIDs->PMT2_Efficiency, fEff[1]);
```

## vector column について

以下の vector column は、作成時に vector 参照を渡している。

```cpp
CreateNtupleDColumn("PMT1_HitTimes", fEventAction->GetHitTimeListRef(0));
```

この形式では、毎イベント `FillNtupleDColumn()` で値を詰める必要はない。

ただし、戻り値の ID は保存しておいた方がよい。

理由:

- コメントの ID ずれを防げる。
- 将来、vector column を削除・移動・追加したときの確認が簡単になる。
- 出力 column 一覧をコード上で一元管理できる。

## 実装手順

### Step 1: ID 保持用ヘッダを追加

`include/AnalysisColumnIDs.hh` を作る。

すべての ntuple column に対応する `G4int` メンバを用意し、初期値は `-1` にする。

### Step 2: RunAction で ID を保存

`RunAction.hh` に `AnalysisColumnIDs fColumnIDs;` を追加する。

`RunAction.cc` の `CreateNtuple...Column()` の戻り値を、すべて `fColumnIDs.xxx` に代入する。

### Step 3: EventAction に ID を渡す

`EventAction.hh` に以下を追加する。

```cpp
void SetColumnIDs(const AnalysisColumnIDs* ids);
const AnalysisColumnIDs* GetColumnIDs() const;
```

`RunAction` コンストラクタの最後、または column 作成後に以下を呼ぶ。

```cpp
fEventAction->SetColumnIDs(&fColumnIDs);
```

### Step 4: EventAction の固定 ID を置き換える

`EventAction.cc` の `FillNtupleDColumn(5, ...)` などを `fColumnIDs->...` に置き換える。

### Step 5: ScintiSD の固定 ID を置き換える

`ScintiSD.cc` の `0`, `1`, `2` を `ids->Scinti_Edep` などに置き換える。

### Step 6: CathodeSD の固定 ID を置き換える

`CathodeSD.cc` の `6`, `8`, `10` を `ids->PMT_SUM_Photons` などに置き換える。

このとき、photon count は `FillNtupleIColumn()` を使う。

### Step 7: コメントを ID 番号依存から名前依存に変更

以下のようなコメントは、列追加時にずれやすい。

```cpp
// [6]
// ID: 6 ~ 11
```

推奨コメント:

```cpp
// PMT photon count columns
// IDs are stored in fColumnIDs.
```

または日本語で、

```cpp
// column ID は fColumnIDs に保存して Fill 側で参照する
```

## 注意点

### 注意 1: BuildForMaster の EventAction

`ActionInitialization::BuildForMaster()` でも `EventAction` と `RunAction` を作っている。

master thread 側で ntuple column を作るときにも `SetColumnIDs()` が呼ばれるか確認する必要がある。

### 注意 2: nullptr ガード

`ScintiSD` や `CathodeSD` から `EventAction` を取得する場合、念のため以下を確認する。

```cpp
if (!eventAction) return;
auto ids = eventAction->GetColumnIDs();
if (!ids) return;
```

### 注意 3: 型を合わせる

`CreateNtupleIColumn()` で作った column には `FillNtupleIColumn()` を使う。

`CreateNtupleDColumn()` で作った column には `FillNtupleDColumn()` を使う。

今回特に注意が必要な column:

```text
PMT_SUM_Photons
PMT1_Photons
PMT2_Photons
```

これらは photon count なので `IColumn` が自然。

### 注意 4: vector column は Fill しない

vector 参照を渡して作った column は、`AddNtupleRow()` 時に現在の vector 内容が保存される。

そのため、hit time や hit position vector に対して `FillNtupleDColumn()` を追加で呼ぶ必要はない。

## 将来的な改善案

今回の `AnalysisColumnIDs` 方式は、現在の構成を大きく変えずに ID ずれを減らすための現実的な改善である。

さらに整理したくなった場合は、次の段階として `AnalysisManagerHelper` のようなクラスを作り、column 作成と Fill 処理を完全に 1 箇所に集約する。

将来案:

```cpp
analysis->FillScinti(fTotalEdep, fGeneratedPhotons, fHitCount);
analysis->FillPMTPhotons(fPhotonCount[0], fPhotonCount[1]);
analysis->FillEfficiencies(fRadius, fEff[0], fEff[1]);
```

この形にすると、`ScintiSD`、`CathodeSD`、`EventAction` は column ID を一切知らなくてよくなる。

## 確認項目

実装後に確認すべきこと:

1. `CreateNtuple...Column()` の戻り値がすべて保存されている。
2. `FillNtuple...Column()` に手書き ID が残っていない。
3. `IColumn` には `FillNtupleIColumn()`、`DColumn` には `FillNtupleDColumn()` を使っている。
4. vector column に対して不要な `FillNtupleDColumn()` をしていない。
5. `AddNtupleRow(0)` はイベントの最後に 1 回だけ呼ばれている。
6. `RunAction.cc` のコメントが固定 ID に依存していない。
7. PMT の合計 photon 数が `PMT1 + PMT2` と一致する。
8. PMT の合計 efficiency が `PMT1_Efficiency + PMT2_Efficiency` と一致する。

## 参考にする Geant4 example

Geant4 11.2.2 の以下の example が参考になる。

```text
/home/hashizume/Geant4/geant4-11.2.2/examples/extended/electromagnetic/TestEm9
```

特に以下のファイル:

```text
examples/extended/electromagnetic/TestEm9/src/Histo.cc
examples/extended/electromagnetic/TestEm9/include/Histo.hh
```

TestEm9 では、`CreateNtupleIColumn()` や `CreateNtupleDColumn()` の戻り値を `fTupleI`、`fTupleD` に保存し、Fill 時には保存した ID を使っている。

この考え方を現在のプロジェクトに合わせて小さく導入するのが、今回の計画である。
