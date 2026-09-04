# `include/`・`src/` 全体コード点検レポート

- 点検日: 2026-07-27
- 対象: `include/`・`src/` の全 83 ファイル、合計 5,696 行
- 補助的に確認したもの:
  - `sim.cc`
  - `CMakeLists.txt`
  - 既存の実行ログ `logs/run_20260722_004334_run000/stdout.txt`
  - 既存の ROOT 出力 `root/run_20260722_004334_run000.root`
  - Geant4 11.2.2 の本体実装と公式 example
- 点検方法: 静的コードレビュー、および既存出力の読み取り
- 実施していないもの: ソースコード変更、ビルド、テスト、新規シミュレーション

> [!IMPORTANT]
> 現在の UROKO 1 台・1 MeV 中性子・`+x` 入射という既定経路について、直ちにクラッシュすると断定できる箇所は見つからなかった。一方で、物理リスト、PMT の「到達」と「検出」の定義、無反応イベントの半径、複数検出器の集計方法には、定量評価前に修正または定義の明文化が必要である。
>
> また、HILE、Floor、HPGe、複数台 UROKO など、現在無効な構成には、有効化前に必ず直すべき重大な問題がある。

## 0. 判定基準

### 優先度

| 優先度 | 意味 |
|---|---|
| P0 | ビルド不能、未定義動作、Geant4 の不正なジオメトリなどにつながり得る。該当構成を使う前に必須修正 |
| P1 | 実行はできても、物理結果や解析結果の意味を変える。定量評価前に修正または検証が必要 |
| P2 | 現在の結果を直ちに壊さないが、拡張性、再現性、保守性のために修正を推奨 |
| P3 | 主に命名、重複、整理に関する改善 |

### 到達性

| 表記 | 意味 |
|---|---|
| 現行 | 現在の UROKO 1 台構成でも通る経路 |
| 条件付き | detector mode、`nObj`、物理設定などを変更した場合に通る経路 |
| 休眠 | 現在未使用、またはコメントアウトされているコード |

## 0.1 総合結論

最優先事項は次のとおりである。

1. `PhysicsList` で `G4EmLivermorePhysics` が実際には登録されていない問題を直す。
2. PMT の「光子到達数」と `G4OpBoundaryProcess` による「検出数」を分離する。
3. 中性子無反応イベントで `Scinti_HitPos_Radius` が巨大な値になる問題を直す。
4. 複数台 UROKO を使う前に、検出器 ID ごとの集計へ変更する。
5. HILE と Floor の母娘ボリューム包含関係を直す。
6. HPGe・Shield などの実行時サイズ配列を `std::array` または `std::vector` に置き換える。
7. UROKO 以外を使う場合は、SD、光学物性、磁場、出力列を個別に完成させる。

## 0.2 現在の中性子相互作用数ロジックに対する結論

### 結論

現在の `ScintiSD.cc` は、全 step を無条件に数えているわけではない。

`src/ScintiSD.cc:46-81` では、次をすべて満たす step だけを 1 回として数えている。

1. track が中性子である。
2. `ParentID == 0`、すなわち一次粒子である。
3. post-step を決めた process が存在する。
4. process type が `fHadronic` である。

したがって、現在の定義は次の式に相当する。

```text
Scinti_InteractionCount
  = 検出器内において、一次中性子の step を終了させた
    hadronic process の発生回数
```

これは単なる

```text
全 step 数
```

ではない。Transportation、ジオメトリ境界、電磁過程、ユーザーが細分化しただけの step は除外される。この意味では、

```text
一次中性子が検出器内で起こした hadronic 相互作用回数
  = 現在の条件を満たす step の数
```

という考え方は概ね正しい。

### ただし、定義上の注意

- 二次中性子の相互作用は数えない。
- 1 event に一次中性子が複数ある場合は、それらを合算する。
- elastic、inelastic、capture、fission などを一つの列に合算する。
- `process->GetProcessType() == fHadronic` という広い条件なので、将来 physics list を変えたときに対象 process が増える可能性がある。
- 「検出回数」「ヒット回数」という名称より、「一次中性子 hadronic 相互作用回数」と呼ぶ方が正確である。

### 既存出力の確認

`root/run_20260722_004334_run000.root` の 20,000 event を読み取った結果は次のとおりだった。

| 対象 | 平均 | 中央値 | 90%点 | 最大値 |
|---|---:|---:|---:|---:|
| 全 event | 5.746 | 2 | 16 | 142 |
| `Scinti_Edep != 0` の event | 7.904 | 4 | 19 | 142 |

- 相互作用数が 1 回以上の event は 14,528 event だった。
- `analysis/PlotFigure.C:26` では先に `Scinti_Edep != 0` を課しているため、表示される相互作用数分布は全 event の分布ではなく条件付き分布である。
- 最初の散乱後に中性子が減速すると断面積が変化し、検出器内をランダムウォークして多数回散乱する event が生じる。そのため、数十回から 100 回超の tail が存在すること自体は、直ちに step の過剰計数を意味しない。

### 推奨する検証列

現在の合計列を残したまま、少なくとも次を別々に出力することを推奨する。

```text
PrimaryNeutron_ElasticCount
PrimaryNeutron_InelasticCount
PrimaryNeutron_CaptureCount
PrimaryNeutron_FissionCount
PrimaryNeutron_OtherHadronicCount
```

process 名だけでなく `GetProcessSubType()` も記録すると、physics list の変更に強くなる。Geant4 11.2.2 の `G4HadronicProcessType.hh` には、例えば次が定義されている。

```cpp
fHadronElastic
fNeutronGeneral
fHadronInelastic
fCapture
fFission
```

ただし `fNeutronGeneral` を有効にした構成では内部 process の扱いが変わるため、いきなり subtype の allow-list だけに変更せず、最初に process 名と subtype の実測内訳を確認すべきである。

Geant4 公式 example でも post-step を決めた process は次の形で取得している。

- `examples/extended/hadronic/Hadr04/src/SteppingAction.cc:48-56`
- `examples/extended/hadronic/Hadr03/src/SteppingAction.cc`

## 1. プログラムを作成する上で致命的なエラーになりうる箇所

### 1.1 HILE の娘ボリュームが母ボリューム外へ出る

- 優先度: **P0**
- 到達性: **条件付き**
- 該当箇所:
  - `src/LogVol/HILELogVol.cc:56-79`
  - `src/LogVol/HILELogVol.cc:149-168`

母ボリュームは次の half size で作られている。

```cpp
G4Box(Name+"_Solid", 50.0*mm, 100.0*mm, 350.0*mm);
```

一方、シンチレータは半径 700–712.5 mm、角度 12.3 度の円弧であり、x 方向に平行移動しても y 方向の最大値は概算で

```text
712.5 mm × sin(12.3 deg) ≈ 151.8 mm
```

となる。母ボリュームの y half size 100 mm を超えている。

PMT についても transform から中心はおよそ y = -157.5 mm、長手方向の範囲はおよそ

```text
y = -265 mm ～ -50 mm
```

となり、母ボリュームの y 範囲 `-100 mm ～ +100 mm` を大きく外れる。

これは単なる overlap ではなく、娘ボリュームが母ボリュームに包含されない不正なジオメトリである。HILE を有効にする前に、全 daughter の bounding range から mother size を再計算する必要がある。

### 1.2 Floor が World の外へ出る

- 優先度: **P0**
- 到達性: **条件付き**
- 該当箇所:
  - `src/DetectorConstruction.cc:105-109`
  - `src/DetectorConstruction.cc:400-407`

World は一辺 4 m、すなわち y 範囲 `-2 m ～ +2 m` である。

Floor は厚さ 1 m、中心 y = -2.2 m に配置されるため、

```text
Floor の y 範囲 = -2.7 m ～ -1.7 m
```

となり、0.7 m が World の外へ出る。

さらに x、z の幅として World と同じ 4 m を渡しているため、x、z 面も World 境界と完全に一致する。World を十分な margin 付きで拡大するか、Floor の寸法と位置を見直す必要がある。

### 1.3 実行時 `nObj` と固定長配列が安全に対応していない

- 優先度: **P0**
- 到達性: **条件付き。一部はビルド時にも問題化し得る**
- 該当箇所:
  - `src/DetectorConstruction.cc:171-182`
  - `src/DetectorConstruction.cc:197-245`
  - `src/DetectorConstruction.cc:274-331`
  - `src/DetectorConstruction.cc:374-394`

代表例は UROKO のテスト配置である。

```cpp
const G4int nObj = Mode[objName].nObj;
G4ThreeVector pos_UROKO[] = { ... };  // 要素数 1
G4RotationMatrix rot_UROKO[1];

for (G4int i = 0; i < nObj; i++) {
    pos_UROKO[i] ...
}
```

`Mode["UROKO"].nObj` を 2 以上に変えるだけで、配列外アクセスになる。

HILE の production 配列は 6、Shield の角度は 6、HPGe の初期値は 7 個に固定されているが、loop 上限は `Mode` から独立に得ている。この二重管理は危険である。

また、次のような配列は標準 C++ の可変長配列ではない。

```cpp
G4String GeName[nObj] = { ... };
G4double AngleHPGe[nObj] = { ... };
G4double Angle[nObj] = { ... };
```

標準 C++ ではコンパイルできず、GNU 拡張でも可変長配列の初期化は処理系依存または拒否され得る。

推奨形は次のいずれかである。

```cpp
constexpr std::array<DetectorPlacement, 7> hpgePlacements = { ... };
```

または

```cpp
std::vector<DetectorPlacement> placements = { ... };
```

loop 上限は `placements.size()` から得て、`Mode.nObj` と一致しなければ `G4Exception` で停止させるべきである。

### 1.4 HPGe の未知の名前で未初期化ポインタが使われる

- 優先度: **P0**
- 到達性: **条件付き**
- 該当箇所:
  - `include/LogVol/HPGeLogVol.hh:15-18`
  - `src/LogVol/HPGeLogVol.cc:101-108`
  - `src/DetectorConstruction.cc:327-331`

未知の detector 名が渡されると、constructor は warning を出してそのまま `return` する。

```cpp
if (!GePar.count(Name)) {
    G4cout << "...";
    return;
}
```

この時点で `Solid` と `LogVol` は初期化されていない。その後 `GetLogicalVolume()` が呼ばれると不定値ポインタを返し、未定義動作になる。

少なくともメンバを `nullptr` 初期化し、未知の名前は `FatalException` で fail-fast にする必要がある。

### 1.5 休眠 utility には、そのまま再利用できない未定義動作がある

- 優先度: **P0**
- 到達性: **休眠**
- 該当箇所:
  - `include/Material/util/atom.hh`
  - `include/Material/util/ATTEN2D.h`
  - `include/Material/util/ECONVO.h`
  - `include/Material/util/EEQUIV.h`
  - `include/Material/util/col.hh`

#### `atom.hh`

`Atom_Name()` はローカル配列のアドレスを返している。

```cpp
char ret[10];
return ret;
```

関数を抜けた直後に無効になるため、使用すると未定義動作である。また、原子番号 53 は iodine の `"I"` ではなく `"In"` になっている。

#### `ATTEN2D.h`

- include guard がない。
- header 内に non-inline の関数とグローバル配列を定義しており、複数 translation unit から include すると多重定義になる。
- `numX()` の loop 条件と `numx + 1` の組合せには配列外参照の余地がある。
- 読み込み path は `../include/util/ATTEN2D.txt` だが、実ファイルは `include/Material/util/ATTEN2D.txt` にある。
- 入力行数に対する bounds check がない。
- `fclose()` がない。
- 初期化フラグとグローバル配列が thread-safe ではない。

#### `ECONVO.h`

- C の `rand()`、`srand()`、`time()` を使い、Geant4 の乱数系列から外れる。
- MT でデータ競合と非再現性を生む。
- `urand() == 0` の場合、Box–Muller 変換で `log(0)` になる。

#### `EEQUIV.h`・`col.hh`

- header 内のグローバル変数、non-inline 関数により ODR 違反になり得る。
- `EEQUIV()` は `Erecoil <= 0` で `log10()` の定義域外になる。
- `col.hh` は `std::map` を使うが `<map>` を直接 include していない。

これらは現在の `include/src` から呼ばれていないため現行結果には影響しない。再利用する場合は、個別に修理するより、必要な式とデータだけを新しい `.hh/.cc` に移植する方が安全である。

### 1.6 Geant4 store 所有オブジェクトを wrapper destructor で削除している

- 優先度: **P1**
- 到達性: **潜在的**
- 該当箇所:
  - すべての `src/LogVol/*LogVol.cc` の destructor

多くの destructor が次の形になっている。

```cpp
delete Solid;
delete LogVol;
```

Geant4 の solid、logical volume、physical volume は store に登録され、通常は Geant4 kernel が一括管理する。現在は `DetectorConstruction` 側で各 wrapper 自体を解放していないため、この destructor はほぼ呼ばれず、問題が隠れている。

将来 wrapper を `delete` または `std::unique_ptr` で正しく解放すると、store 側に dangling pointer を残し、終了時や geometry reinitialization 時に二重削除へつながり得る。

wrapper は Geant4 オブジェクトを non-owning pointer として持ち、destructor では削除しない方針に統一すべきである。

### 1.7 PMT index API に bounds check がない

- 優先度: **P1**
- 到達性: **潜在的**
- 該当箇所:
  - `include/EventAction.hh:19-29`
  - `include/CathodeSD.hh:17-22`

現在の `CathodeSD.cc:48-50` は index を 0 または 1 に制限しているため、現行経路は安全である。一方、公開 getter と追加関数は任意の `id` で固定長配列へアクセスできる。

PMT 数を増やす前に、`std::array` と `.at()`、または detector/PMT ID を key とするコンテナへ変更すべきである。

### 1.8 macro 実行失敗と出力ファイル作成失敗を検出していない

- 優先度: **P1**
- 到達性: **現行**
- 該当箇所:
  - `sim.cc:139-142`
  - `src/RunAction.cc:37-50`
  - `sim.cc:30-53`

`ApplyCommand()`、`OpenFile()`、ログ用 `ofstream` の結果を確認していない。そのため、macro の構文エラー、存在しない macro、出力 directory 不在などが起きても、プロセスが成功終了したように見える可能性がある。

シミュレーションの自動実行では、これは「結果ファイルがない、または不完全なのにジョブ成功と判定される」という運用上の致命的問題になり得る。

## 2. ビルドは通るが物理的に正しくない結果につながりうる箇所

### 2.1 `G4EmLivermorePhysics` は現在有効になっていない

**処理済み**

- 優先度: **P1**
- 到達性: **現行**
- 該当箇所: `src/PhysicsList.cc:6-11`

`FTFP_BERT_HP` は constructor 内ですでに次を登録している。

```text
G4EmStandardPhysics
G4DecayPhysics
G4RadioactiveDecayPhysics
```

その上で現在のコードは次を呼んでいる。

```cpp
RegisterPhysics(new G4RadioactiveDecayPhysics());
RegisterPhysics(new G4EmLivermorePhysics());
```

既存ログ `logs/run_20260722_004334_run000/stdout.txt:16-25` には、次が明記されている。

```text
existing physics is G4EmStandard
New G4EmLivermore can not be registered
Duplicate type for G4EmLivermore
```

したがって、「Livermore を追加したつもり」でも、physics constructor として実際に残るのは `G4EmStandardPhysics` である。

Geant4 11.2.2 の標準 EM も一部の光子過程に Livermore model を使用するため、「全てが別モデルになっている」とまでは言えない。しかし、`G4EmLivermorePhysics` を選んだことにはなっておらず、意図と実際の構成が一致していない。

Livermore へ切り替える意図なら、公式 example と同様に次を使う。

```cpp
PhysicsList::PhysicsList()
  : FTFP_BERT_HP()
{
    ReplacePhysics(new G4EmLivermorePhysics());
    RegisterPhysics(new G4OpticalPhysics());
}
```

参考:

- `examples/extended/medical/GammaTherapy/src/PhysicsList.cc:137-159`
- `examples/extended/optical/wls/wls.cc:68-77`

### 2.2 `G4RadioactiveDecayPhysics` が二重登録されている

- 優先度: **P1**
- 到達性: **現行**
- 該当箇所: `src/PhysicsList.cc:8`

`FTFP_BERT_HP` にすでに含まれるため、追加登録は不要である。

既存ログ `stdout.txt:173-191` では `Radioactivation` process の重複が検出され、二つ目は追加されていない。このログに基づく限り、同じ崩壊が二重に実行されているわけではないが、初期化時の warning と physics configuration の不透明さを生んでいる。

`RegisterPhysics(new G4RadioactiveDecayPhysics())` は削除すべきである。

### 2.3 PMT の「到達光子」と「検出光子」が分離されていない

- 優先度: **P1**
- 到達性: **現行**
- 該当箇所:
  - `src/CathodeSD.cc:29-65`
  - `include/CathodeSD.hh:17-25`
  - `src/Surface/UROKO/CathodeSurface.cc:13-18`
  - `src/EventAction.cc:38-44`

`CathodeSD` は、

```text
post volume の名前が Cathode
```

であれば `fPhotonArrivedCount` を増やし、光子を無条件に kill する。一方、`G4OpBoundaryProcess::GetStatus() == Detection` は確認していない。

さらに `fPhotonDetectedCount` と `fBoundary` は宣言されているが、一度も更新・使用されない。そのため `GetDetectedPhotons()` は常に 0 を返す。

現在の Cathode surface は

```text
REFLECTIVITY = 0
EFFICIENCY   = 1
```

という理想光電面なので、表面へ入射した光子は boundary process 上も原則として全て `Detection` になる。この限定されたモデルでは「到達数」と「検出数」が数値的に一致しやすい。

しかし量子効率を実値へ変更した瞬間、現在の SD は次の誤りを起こす。

- 未検出の吸収光子も検出数として数える。
- 反射されるはずの光子も数えて強制的に kill する可能性がある。
- `PMT*_Efficiency` が実際の検出効率ではなく、光学輸送効率と理想 QE の積になる。

推奨する列は次のとおりである。

```text
PMT*_ArrivedPhotons
PMT*_DetectedPhotons
PMT*_CollectionEfficiency = Arrived / Generated
PMT*_DetectionEfficiency  = Detected / Generated
PMT*_ConditionalQE        = Detected / Arrived
```

検出判定には `G4OpBoundaryProcessStatus::Detection` を使う。参考になる公式 example は次のとおりである。

- `examples/extended/optical/OpNovice2/src/SteppingAction.cc:178-230`
- `examples/extended/optical/LXe/src/LXeSteppingAction.cc:184-201`
- `examples/extended/optical/wls/wls.cc:73-76`
- `examples/extended/optical/wls/src/WLSSteppingAction.cc:318-325`

### 2.4 無反応 event の hit radius が巨大な有限値になる

- 優先度: **P1**
- 到達性: **現行**
- 該当箇所:
  - `src/ScintiSD.cc:21-23`
  - `src/EventAction.cc:50-57`

中性子相互作用がない場合、local position は

```text
(-99999, -99999, -99999)
```

のままである。しかし `EventAction` は相互作用の有無を確認せず、

```cpp
sqrt(posL.x()*posL.x() + posL.y()*posL.y())
```

を計算するため、radius は約 141,420 mm になる。

この値は NaN でも共通 sentinel でもなく、「非常に外側で相互作用した event」に見えるため、ヒストグラムや平均値を汚染する。

`HasFirstInteraction()` のような状態を SD から取得し、無反応なら radius も一貫した sentinel または `NaN` にすべきである。解析側では `InteractionCount > 0` を明示的に条件にする。

### 2.5 4 台 UROKO では全検出器の応答が一つに合算される

- 優先度: **P1**
- 到達性: **条件付き**
- 該当箇所:
  - `src/DetectorConstruction.cc:138-167`
  - `src/DetectorConstruction.cc:443-460`
  - `src/CathodeSD.cc:45-58`

production block は一つの `UROKO_LogVol` を 4 回配置する設計である。これは geometry の共有としては通常の方法だが、同じ logical volume に一つの `ScintiSD` と一つの `CathodeSD` を付けているため、event 内の 4 台分の値が同じ accumulator に入る。

さらに PMT の copy number は各 UROKO 内で常に 1 と 2 であり、外側 UROKO placement の copy number を読んでいない。

したがって現在の出力では、

- どの UROKO で相互作用したか
- どの UROKO の PMT1/PMT2 に届いたか
- detector ごとの効率

を区別できない。

`Touchable` の階層から UROKO placement の copy number と PMT copy number の両方を取り出し、

```text
[detectorIndex][pmtIndex]
```

で集計する必要がある。

### 2.6 UROKO 以外の detector mode は「配置できる」だけで、検出器応答が未完成

- 優先度: **P1**
- 到達性: **条件付き**
- 該当箇所: `src/DetectorConstruction.cc:437-466`

SD が実際に attach されるのは UROKO の scintillator と PMT だけである。

- LigGlass: SD attach がコメントアウトされている。
- HILE: SD attach がコメントアウトされている。
- HPGe: crystal 用 SD と解析出力がない。
- BetaPlastic: scintillator 用 SD と解析出力がない。
- Magnet、Frame、Floor、Shield、Chamber、Stopper: passive geometry としてのみ存在する。

したがって `Mode` を true に変えるだけでは、その detector の応答は ROOT tree に記録されない。

### 2.7 LigGlass と HILE の光学モデルが未完成

- 優先度: **P1**
- 到達性: **条件付き**
- 該当箇所:
  - `src/Material/GS20Mat.cc:44-54`
  - `src/LogVol/HILELogVol.cc:100-133`

GS20 には組成と密度だけがあり、次が定義されていない。

```text
RINDEX
ABSLENGTH
SCINTILLATIONCOMPONENT
SCINTILLATIONYIELD
SCINTILLATIONTIMECONSTANT
Birks constant または粒子種別 yield
```

このままでは OpticalPhysics を登録していても GS20 から scintillation photon は生成されない。

HILE の acrylic と Pyrex は NIST material をそのまま使っており、光学物性を付加していない。Optical photon が `RINDEX` のない material 境界へ到達すると、`G4OpBoundaryProcess` は `NoRINDEX` として光子を停止する。

UROKO 用 `ScintiSD` をそのまま流用するだけでは、Li glass 固有の反応と発光モデルにもならない。`6Li(n,α)t` の energy deposition、quenching、光量をどのレベルでモデル化するかを先に定義すべきである。

### 2.8 Magnet geometry はあるが磁場がない

- 優先度: **P1**
- 到達性: **条件付き**
- 該当箇所:
  - `src/LogVol/MagnetLogVol.cc`
  - `src/DetectorConstruction.cc:437-466`

NEOMAX と鉄の geometry/material は定義されているが、`G4MagneticField`、`G4FieldManager`、field map、equation of motion は存在しない。

したがって現在の Magnet は、粒子を曲げる磁石ではなく、物質との相互作用だけを起こす passive object である。これが意図なら明記し、磁場を期待しているなら別途実装が必要である。

### 2.9 BC408 の rise time 0.9 ns は現在使われない

- 優先度: **P1**
- 到達性: **現行**
- 該当箇所:
  - `src/Material/BC408Mat.cc:59-65`
  - `src/PhysicsList.cc:11`

`SCINTILLATIONRISETIME1 = 0.9 ns` は material table に登録されているが、Geant4 11.2.2 の `G4OpticalParameters` では finite rise time の default は false である。

したがって現在の PMT hit time には 2.1 ns の decay time は使われるが、0.9 ns の rise time は使われない。

rise time を含める意図なら、初期化前に次を設定する。

```cpp
G4OpticalParameters::Instance()
    ->SetScintFiniteRiseTime(true);
```

または macro で次を指定する。

```text
/process/optical/scintillation/setFiniteRiseTime true
```

参考:

- `examples/extended/optical/OpNovice2/electron.mac:48`

### 2.10 `DielectricSurface` の `REFLECTIVITY` の説明と実際の意味が異なる

- 優先度: **P2**
- 到達性: **現行**
- 該当箇所: `src/Surface/UROKO/DielectricSurface.cc:7-18`

コメントでは `REFLECTIVITY = 0.999` を「鏡面反射率 99.9%」としているが、`polished`・`dielectric_dielectric` の場合、Geant4 11.2.2 では概ね次の順で処理される。

1. `REFLECTIVITY` の確率で表面損失を免れる。
2. その後、両 material の `RINDEX` を用いて Fresnel 反射・屈折を計算する。

したがって現在の設定は、「99.9% を鏡面反射する」のではなく、「0.1% を表面吸収し、残りに通常の Fresnel 計算を行う」に近い。

光学グリースを表現するなら、コメントを修正した上で、次のどちらを採用するか明確にすべきである。

- 接触損失だけを surface probability として与える。
- 実際の grease layer を volume として作り、その `RINDEX` と `ABSLENGTH` を与える。

### 2.11 HPGe・Magnet・配置パラメータに未検証の仮値が含まれる

- 優先度: **P1**
- 到達性: **条件付き**
- 該当箇所:
  - `src/LogVol/HPGeLogVol.cc:79-95`
  - `src/LogVol/MagnetLogVol.cc:233-238`
  - `src/DetectorConstruction.cc:297-305`
  - `src/DetectorConstruction.cc:349-356`

コード中に次が明記されている。

- Kyudai80 の crystal size は別 detector の仮値。
- SUNY 系の複数 detector は値が不明で「適当」な値。
- Magnet の後半厚さは仮値。
- HPGe CINDY の距離は暫定値。
- BetaPlastic は仮に原点へ配置。

これらの mode から得た定量結果は、geometry calibration が完了するまで物理結果として扱うべきではない。

### 2.12 旧 `SensitiveDetector` は再有効化すると相互作用数を二重計数する

- 優先度: **P1**
- 到達性: **休眠**
- 該当箇所: `src/SensitiveDetector.cc:113-177`

旧実装では、境界 step 以外の中性子 step に対してまず無条件に

```cpp
fHitCounter++;
```

し、elastic/inelastic ならさらに 1 回、capture ならさらに 1 回増やす。

つまり実際の elastic、inelastic、capture は最低 2 回として数えられる。Transportation 等も条件によって 1 回と数えられる。

これは今回問題となった「step 数と相互作用数の混同」を実際に含む実装である。現在は attach がコメントアウトされているため影響しないが、再利用せず削除または archive 化するのが安全である。

### 2.13 コメントアウト中の optical photon 強制 kill は、再有効化すると bias を作る

- 優先度: **P1**
- 到達性: **休眠**
- 該当箇所: `src/SteppingAction.cc:13-55`

現在は全体がコメントアウトされているため影響しない。

再有効化した場合の問題は次のとおりである。

- 500 step 超で全 optical photon を kill すると、PMT 到達効率を人工的に下げる。
- `zeroStepCount` は非ゼロ長 step で reset されないため、「連続 zero step」ではなく同じ track 内の累積になり得る。
- Track ID は event ごとに再利用されるが、`thread_local` 変数は event をまたぐ。
- コメントは「4 回連続」と書かれているが、条件は 24 回である。

geometry trap は、まず surface normal、重なり、接触面、boundary status を記録して原因を直すべきであり、固定 step 数 kill は最後の安全策として別カウンタ付きで評価する必要がある。

### 2.14 放射性崩壊の時間閾値は acquisition window と同じ意味ではない

- 優先度: **P2**
- 到達性: **現行だが、主に放射線源 mode**
- 該当箇所: `sim.cc:109-110`

```cpp
SetTimeThresholdForRadioactiveDecay(1.0e+60 * year);
```

は非常に長寿命の daughter まで同一 event 内で追跡対象にし得る。放射性崩壊 chain を最後まで追う意図なら理解できるが、実験の DAQ time window を再現する設定ではない。

Cs-137、Sr-90、Y-90 の source mode では、

- どこまでの daughter を同一 event に含めるか
- detector time window を何 ns、µs、s とするか
- delayed signal を PMT hit time に含めるか

を別々に定義すべきである。

### 2.15 source 設定と particle gun の粒子数が二重管理されている

- 優先度: **P2**
- 到達性: **条件付き**
- 該当箇所:
  - `src/PrimaryGenerator.cc:6-10`
  - `src/RunConfig.cc:9-13`
  - `include/RunConfig.hh:23`

`RunConfig` は `fParticlesPerEvent` を持つが、particle gun は常に

```cpp
new G4ParticleGun(1);
```

である。現在値はどちらも 1 なので結果は一致しているが、`RunConfig` 側だけ変更しても生成粒子数は変わらない。

1 event 1 primary を設計上固定するなら、未使用設定を削除する。可変にするなら gun constructor に反映させるべきである。

### 2.16 World の `G4_AIR` に optical property がない

- 優先度: **P2**
- 到達性: **現行だが影響は限定的**
- 該当箇所:
  - `src/DetectorConstruction.cc:99-108`
  - `src/Material/AirMat.cc`

custom `AirMat` には `RINDEX` があるが、World は NIST `G4_AIR` を直接使っている。

UROKO 内部の母ボリュームは custom `VacuumMat` なので主要な光輸送は成立するが、UROKO から World へ出た optical photon は `RINDEX` のない境界で停止する。外へ逃げた光子を捨てるモデルなら実害は小さいが、外部反射や再入射を扱うモデルにはなっていない。

## 3. 物理的には正しいが、コードの可読性を向上させる上で修正すべき箇所

### 3.1 detector 構成と placement data をコード本体から分離する

- 優先度: **P2**
- 該当箇所: `src/DetectorConstruction.cc`

`DetectorConstruction.cc` は mode 切替、geometry construction、placement data、test/production のコメントブロックを一つに持っている。

次のような構造体を detector ごとに用意すると、`nObj` と配列の不一致を防ぎやすい。

```cpp
struct DetectorPlacement {
    G4String modelName;
    G4int copyNumber;
    G4Transform3D transform;
};
```

test と production はコメントアウトで切り替えず、設定オブジェクトまたは macro command から明示的に選ぶ方がよい。

### 3.2 member pointer と local variable の shadowing をなくす

- 優先度: **P2**
- 該当箇所:
  - `include/DetectorConstruction.hh:34-44`
  - `src/DetectorConstruction.cc:310`
  - `src/DetectorConstruction.cc:338-425`

header には `fHPGe`、`fMagnet`、`fFrame` 等があるが、constructor で初期化されず、`Construct()` 内では同名 local variable が宣言されている。

例:

```cpp
HPGeLogVol* fHPGe[nObj];
MagnetLogVol* fMagnet = new MagnetLogVol(...);
```

これは member が更新されたように見えて実際には更新されない。現在使用しない member は削除し、必要なものは container member として明示的に保持すべきである。

### 3.3 material と optical surface を一元管理する

- 優先度: **P2**
- 該当箇所:
  - `src/LogVol/UROKOLogVol.cc:133-143`
  - 各 `src/LogVol/*LogVol.cc`
  - `src/Material/*.cc`

各 LogVol constructor が同名 material を毎回 `new` するため、複数 detector type を有効にすると `"BC408"`、`"MgO"` 等の同名 material が複数 table に登録され得る。

特に GS20 内部の `"MgO"` と `MgOMat` の `"MgO"` は名前が重複する。

material factory または registry を一つ作り、

```text
同じ物理 material = 同じ G4Material*
```

となるようにする方が、physics table、ログ、再初期化、メモリ管理を理解しやすい。

### 3.4 dead code と未使用メンバを整理する

- 優先度: **P2**
- 該当例:
  - `src/SensitiveDetector.cc`
  - `src/SteppingAction.cc`
  - `src/EventAction.cc:28`
  - `src/LogVol/HILELogVol.cc:140-149`
  - `src/LogVol/UROKOLogVol.cc:200-207`

主な例は次のとおりである。

- `SensitiveDetector` は compiled されるが使われない。
- `SteppingAction` は登録されるが実処理は全てコメントアウトされている。
- `EventAction` の `analysisManager` は未使用。
- HILE の `z_scinti`、`z_guide`、`z_pmt`、`rot90X`、`x_shift` は未使用。
- UROKO の `surfTybek`、`surfDiffuse` は作るが surface に割り当てない。
- `CathodeSD::fPhotonDetectedCount` と `fBoundary` は未完成のまま残っている。

休眠コードは将来の参考として source tree に残すより、version control の履歴に任せた方が、現在何が実行されるかを判断しやすい。

### 3.5 include guard とファイル名の誤記を直す

- 優先度: **P2**
- 該当箇所:
  - `include/Material/util/common.hh:1-2`
  - `include/PrimaryGeneratorMessenger.hh:1-2`
  - `src/Material/NeomacMat.cc`

`common.hh` は、

```cpp
#ifndef MY_COMMOM_H
#define MY_COMMON_H
```

となっており、guard が機能していない。

`PRIMARYGENARATORMESSANGER_HH` は macro としては一致しているため動作するが、綴りが誤っている。`NeomaxMat.hh` に対する implementation file が `NeomacMat.cc` なのも検索性を下げる。

### 3.6 使用する標準 library header を直接 include する

- 優先度: **P2**
- 該当箇所: 多数の LogVol、Material、RunAction

現在は Geant4 header の推移的 include に依存して、`std::map`、`std::vector`、`std::pair`、`std::sqrt`、`std::stringstream` 等を使っている箇所が多い。

使用箇所で直接、例えば次を include すべきである。

```cpp
#include <array>
#include <cmath>
#include <map>
#include <sstream>
#include <utility>
#include <vector>
```

Geant4 version や compiler の変更で突然 build が壊れることを防げる。

### 3.7 重複する geometry helper と色 table を共通化する

- 優先度: **P3**
- 該当箇所: ほぼすべての `src/LogVol/*.cc`

`Rotate()`、`Move()`、`Transform()`、`ColID`、`defaultRGB`、`Color()` が detector ごとにコピーされている。

重複により、

- detector ごとに色の定義済み enum が異なる。
- `operator[]` で未登録色を使うと暗黙に黒が生成される。
- helper の引数が不正でも identity transform になる。

といった差が生じる。

共通 helper を header-only にするなら `inline` または `constexpr` を用い、色 table は `.at()` で未登録値を検出する方がよい。

### 3.8 物理量の型、列名、単位を明確にする

- 優先度: **P2**
- 該当箇所:
  - `src/AnalysisOutput.cc`
  - `include/ScintiSD.hh`
  - `include/CathodeSD.hh`

改善候補は次のとおりである。

- `fGeneratedPhotons` は count なので `G4int` または `std::uint64_t`。
- `GetArrivedPhotons()` は内部が `G4int` なのに戻り値が `G4double`。
- `Scinti_HitTime` は first hadronic interaction time であり、一般的な hit time ではない。
- `Scinti_HitPos_*` も first primary-neutron hadronic interaction position と明記する。
- `PMT*_Photons` は arrived か detected かを列名に含める。
- ROOT branch 名または metadata に MeV、mm、ns を記録する。

推奨例:

```text
Scinti_Edep_MeV
Scinti_Evis_MeV
PrimaryNeutron_FirstInteractionTime_ns
PrimaryNeutron_FirstInteractionPosLocal_mm
PMT1_ArrivedPhotonCount
```

### 3.9 magic number を detector parameter 構造体へまとめる

- 優先度: **P2**
- 該当箇所:
  - `src/LogVol/UROKOLogVol.cc`
  - `src/LogVol/HILELogVol.cc`
  - `src/DetectorConstruction.cc`

UROKO の寸法は LogVol と production placement の両方に書かれ、`total_Z` も一方では計算値、他方では `111.5 mm` の hard-code になっている。

寸法、material 名、surface parameter、placement margin は一つの immutable parameter object にまとめるべきである。

### 3.10 CMake を target ベースへ更新する

- 優先度: **P2**
- 該当箇所: `CMakeLists.txt`

現在は `file(GLOB_RECURSE src/*.cc)` を使うため、不要な旧実装も自動的に build 対象になり、新規ファイル追加時の CMake 再構成も分かりにくい。

改善案:

- source を明示列挙する。
- `target_include_directories()` を使う。
- C++ standard を明示する。
- `CMAKE_CXX_EXTENSIONS OFF` で非標準 VLA を検出する。
- compile warnings を有効にする。

例:

```cmake
set_target_properties(sim PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_compile_options(sim PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)
```

### 3.11 run 終了処理を明示する

- 優先度: **P2**
- 該当箇所: `sim.cc:128-161`

`visManager` と `runManager` を削除しないまま `main()` を終了している。

既存ログ末尾には次が出ている。

```text
WARNING - Attempt to delete the physical volume store while geometry closed !
WARNING - Attempt to delete the logical volume store while geometry closed !
WARNING - Attempt to delete the solid store while geometry closed !
WARNING - Attempt to delete the region store while geometry closed !
```

数値結果を直ちに変える問題ではないが、正常終了時にも warning が残ると、本当に重要な warning を見落としやすい。公式 example と同様に、適切な順序で manager を破棄すべきである。

### 3.12 乱数 seed の再利用手段を用意する

- 優先度: **P2**
- 該当箇所: `sim.cc:65-73`

seed はログに保存されるが、次回実行時は常に `std::random_device` から新しい seed を生成する。したがってコメントにある「後で同じ結果を再現」は、コードの hard-code 変更なしには実行しにくい。

例えば次の優先順位にするとよい。

```text
1. 環境変数または UI command で seed が指定されていれば使う
2. 未指定なら random_device から生成する
3. 最終的に使った seed と engine を metadata に保存する
```

MT では thread 数と event scheduling も再現条件に含める必要がある。

### 3.13 `RunConfig` の共有可変状態を整理する

- 優先度: **P2**
- 該当箇所:
  - `src/ActionInitialization.cc`
  - `src/PrimaryGeneratorMessenger.cc`

一つの `RunConfig*` を master と全 worker action に渡している。run 中に read-only なら問題は小さいが、UI messenger は PreInit/Idle で変更可能であり、MT で共有 `G4String` や vector を変更する設計は慎重に扱う必要がある。

推奨方針は次のいずれかである。

- run 開始前に immutable snapshot を worker ごとにコピーする。
- master-owned config を mutex で保護し、worker は run 開始時にコピーする。
- Geant4 の messenger と action initialization の公式 MT pattern に合わせる。

### 3.14 `BuildForMaster()` の dummy `EventAction` の所有権を明示する

- 優先度: **P3**
- 該当箇所: `src/ActionInitialization.cc:16-20`

master 側では vector ntuple column の参照先を用意するために `EventAction` を作っているが、user action として登録されず、解放もされない。

参照寿命を保つために意図的に残しているように見えるが、その意図がコードから分かりにくい。`RunAction` が ownership を持つ、または master 用の明示的な storage object を持つ設計にすると安全である。

## 4. 現在の実装で妥当と確認できた点

問題点だけでなく、次は現在の方針で妥当と判断した。

1. `ScintiSD` の interaction count は raw step count ではなく、一次中性子かつ hadronic process に限定されている。
2. process は post-step の `GetProcessDefinedStep()` から取得しており、Geant4 公式 example と同じ基本方針である。
3. 最初の hadronic interaction position に post-step position を使うのは妥当である。
4. BC408 の波長表を逆順にして photon energy 昇順へ変換する処理は正しい。
5. Birks constant は既存ログで `0.126938 mm/MeV` と確認でき、コード中の換算と整合している。
6. `ScintiSD` の `Edep` は全粒子の energy deposition を合算し、`Evis` は `G4EmSaturation` を通している。
7. scintillation photon 数は creator process が `"Scintillation"` の二次 optical photon に限定されている。
8. UROKO 1 台について、既存ログ上の Bumper、Scinti、Guide、PMT1、PMT2、Cathode、UROKO placement の overlap check はすべて `OK` である。
9. SD の `EndOfEvent()` で値を埋め、`EventAction::EndOfEventAction()` で 1 row を確定する流れは、Geant4 の event lifecycle と整合している。
10. analysis ntuple merging を booking 時に設定し、master/worker で同じ schema を作る基本方針は妥当である。

なお、`src/ScintiSD.cc:65` のコメントは「pre と post の中点」となっているが、実装は post position である。実装が妥当なのでコメントを合わせるべきである。

## 5. 推奨修正順序

### Phase 1: 現在の UROKO 1 台で定量評価を始める前

1. `PhysicsList` の重複登録を除き、Livermore を使うなら `ReplacePhysics()` にする。
2. `Scinti_HitPos_Radius` の無反応 sentinel 問題を直す。
3. PMT arrival と boundary `Detection` を分離する。
4. interaction count を elastic、inelastic、capture、other に分解して検証する。
5. `PlotFigure.C` で全 event 分布と `Edep != 0` 条件付き分布を別々に表示する。
6. macro、ROOT file、log file の失敗を exit status に反映する。

### Phase 2: UROKO 4 台へ進む前

1. touchable depth から UROKO detector ID を取得する。
2. `[detector][pmt]` 単位で count、time、position を保存する。
3. test/production のコメント切替を設定 object に置き換える。
4. fixed array の要素数と placement 数を一つの container に統合する。

### Phase 3: 他 detector を有効にする前

1. HILE mother volume を修正し、全 daughter の containment を確認する。
2. Floor と World の包含関係を修正する。
3. LigGlass、HILE、HPGe、BetaPlastic 用 SD と output schema を設計する。
4. GS20 と HILE の optical property を定義する。
5. Magnet に磁場を入れるか、passive geometry であることを明記する。
6. HPGe、Magnet、detector distance の仮値を実測・図面値へ置き換える。

### Phase 4: 保守性改善

1. 休眠 `SensitiveDetector` と utility header を削除または隔離する。
2. raw pointer ownership を整理する。
3. material/surface factory を作る。
4. CMake、include、include guard、命名を整理する。
5. run metadata と再現用 seed interface を追加する。

## 6. 公式 example で特に参考になるもの

### hadronic process の取得

- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/hadronic/Hadr04/src/SteppingAction.cc`
- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/hadronic/Hadr03/src/SteppingAction.cc`

### EM physics constructor の置換

- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/medical/GammaTherapy/src/PhysicsList.cc`
- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/wls/wls.cc`

### optical boundary の `Detection`

- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/OpNovice2/src/SteppingAction.cc`
- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/LXe/src/LXeSteppingAction.cc`
- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/wls/src/WLSSteppingAction.cc`

`wls` は `G4OpticalParameters::SetBoundaryInvokeSD(true)` を用いる比較的新しい方法、`LXe` は boundary status を見て SD を明示的に呼ぶ方法の参考になる。

## 7. 点検範囲の内訳

| 区分 | ファイル数 | 確認内容 |
|---|---:|---|
| core header/source | 26 | action、generator、physics、SD、analysis、detector construction |
| `LogVol` | 22 | 11 detector/構造物の header と source |
| `Material` | 16 | 8 material class の header と source |
| `Material/util` | 7 | utility header 6 個とデータ 1 個 |
| `Surface/UROKO` | 12 | 6 optical surface の header と source |
| 合計 | 83 | `include/`・`src/` の全ファイル |

## 8. 最終判定

現在の中性子相互作用回数が大きく見える主因を、単純な「step の刻み過ぎ」と判断する根拠はない。現在の `ScintiSD` は hadronic process に限定しており、考え方の中心部分は妥当である。

ただし、定量的に信頼できる値として確定するには、次の二つが必要である。

1. elastic、inelastic、capture、other の内訳を出して、合計値がどの process から構成されているか確認する。
2. 全 event と `Edep != 0` event を分け、条件付き分布だけを見て「回数が大きすぎる」と判断しない。

コード全体としては、現在の UROKO 1 台モードは「検証を続けられる段階」だが、UROKO 4 台および他 detector mode は「有効化前の設計・ジオメトリ修正が必要な段階」である。
