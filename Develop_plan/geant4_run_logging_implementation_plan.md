# Geant4 Run ログ保存機構 実装計画書

## 目的

Geant4 simulation の各 run について、ROOT ファイルだけでなく、実行条件・乱数 seed・Git 状態・Geant4 環境・マルチスレッド設定・実行 macro を保存し、後から結果の比較、再現、原因調査ができるようにする。

現在のコードでは、ROOT ファイルは `src/RunAction.cc` の `BeginOfRunAction()` 内で次のように保存されている。

```cpp
analysisManager->OpenFile("../root/output" + strRunID.str() + ".root");
```

この方式では、run ID が実行ごとに 0 から始まるため、`output0.root` が上書きされやすい。また、ROOT ファイルだけでは「どの粒子・エネルギー・seed・thread 数・Git commit で作ったか」が残らない。

そこで、ROOT ファイルとログディレクトリを同じ `run label` で対応づける。

## 最終的な出力構成

```text
root/
  run_YYYYMMDD_HHMMSS_run000.root

logs/
  run_YYYYMMDD_HHMMSS_run000/
    manifest.json
    parameter.env
    version.txt
    random.txt
    mt.txt
    macro.mac
    physics.txt
    analysis_schema.txt
    output.txt
    stdout.txt
    git.diff
```

例:

```text
root/
  run_20260720_153012_run000.root

logs/
  run_20260720_153012_run000/
    manifest.json
    parameter.env
    version.txt
    random.txt
    mt.txt
    macro.mac
    output.txt
    stdout.txt
    git.diff
```

## 基本方針

- `runID` だけで ROOT ファイル名を作らない。
- `run label` を ROOT ファイル名とログディレクトリ名で共有する。
- 最初は外側 shell script でログ基盤を作る。
- 次に C++ 側へ `RunConfig` と `RunLogger` を導入する。
- マルチスレッド時、ログファイルへの書き込みは master thread のみが行う。
- worker thread は ROOT ntuple の fill に集中させる。
- `PrimaryGenerator` が実際に使った値と、ログに保存する値がずれないようにする。

## 注意する既存構造

現在の `ActionInitialization` は、master 用と worker 用で action の生成が分かれている。

```cpp
void ActionInitialization::BuildForMaster () const
{
    EventAction* masterEventAction = new EventAction();
    RunAction *runAction = new RunAction(masterEventAction);
    SetUserAction(runAction);
}

void ActionInitialization::Build () const
{
    EventAction* eventAction = new EventAction();
    SetUserAction(eventAction);

    PrimaryGenerator *generator = new PrimaryGenerator();
    SetUserAction(generator);

    RunAction *runAction = new RunAction(eventAction);
    SetUserAction(runAction);

    SteppingAction *steppingAction = new SteppingAction();
    SetUserAction(steppingAction);
}
```

Geant4 の MT mode では、master 側には `PrimaryGenerator` が無い場合がある。そのため、master の `RunAction` から直接 `PrimaryGenerator` を読んで run 条件を保存する設計は避ける。

## Phase 1: 外側 run script を作る

### 目的

C++ 側を大きく変更する前に、run ごとのログディレクトリ作成、macro 保存、Git 状態保存、stdout 保存を実現する。

### 追加候補ファイル

```text
scripts/run_with_log.sh
```

### script の役割

1. run label を作る。
2. `root/` を作る。
3. `logs/<run_label>/` を作る。
4. 実行 macro を `logs/<run_label>/macro.mac` にコピーする。
5. Git commit hash と Git status を `version.txt` に保存する。
6. `git diff` を `git.diff` に保存する。
7. ROOT 出力先とログ出力先を環境変数で simulation に渡す。
8. stdout / stderr を `stdout.txt` に保存する。

### script 実装例

```bash
#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <executable> <macro>"
    exit 1
fi

exe="$1"
macro="$2"

timestamp="$(date +%Y%m%d_%H%M%S)"
run_id="000"
run_label="run_${timestamp}_run${run_id}"

project_dir="$(cd "$(dirname "$0")/.." && pwd)"
root_dir="${project_dir}/root"
log_dir="${project_dir}/logs/${run_label}"

mkdir -p "${root_dir}"
mkdir -p "${log_dir}"

root_file="${root_dir}/${run_label}.root"

cp "${macro}" "${log_dir}/macro.mac"

{
    echo "RUN_LABEL=${run_label}"
    echo "ROOT_FILE=${root_file}"
    echo "LOG_DIR=${log_dir}"
    echo "MACRO_FILE=${macro}"
    echo "START_TIME=$(date --iso-8601=seconds)"
} > "${log_dir}/output.txt"

{
    echo "GIT_COMMIT=$(git -C "${project_dir}" rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "GIT_BRANCH=$(git -C "${project_dir}" branch --show-current 2>/dev/null || echo unknown)"
    echo
    echo "[git status --short]"
    git -C "${project_dir}" status --short 2>/dev/null || true
} > "${log_dir}/version.txt"

git -C "${project_dir}" diff > "${log_dir}/git.diff" 2>/dev/null || true

cat > "${log_dir}/manifest.json" <<EOF
{
  "run_label": "${run_label}",
  "root_file": "${root_file}",
  "log_dir": "${log_dir}",
  "macro_file": "${macro}",
  "status": "running"
}
EOF

export G4_RUN_LABEL="${run_label}"
export G4_OUTPUT_ROOT="${root_file}"
export G4_LOG_DIR="${log_dir}"

"${exe}" "${macro}" > "${log_dir}/stdout.txt" 2>&1
status="$?"

{
    echo "END_TIME=$(date --iso-8601=seconds)"
    echo "EXIT_STATUS=${status}"
} >> "${log_dir}/output.txt"

if [ "${status}" -eq 0 ]; then
    sed -i 's/"status": "running"/"status": "completed"/' "${log_dir}/manifest.json"
else
    sed -i 's/"status": "running"/"status": "failed"/' "${log_dir}/manifest.json"
fi

exit "${status}"
```

### この段階のテスト

1. `scripts/run_with_log.sh` を実行して `logs/<run_label>/` が作られることを確認する。
2. `macro.mac` が保存されることを確認する。
3. `version.txt` に Git commit と status が入ることを確認する。
4. `stdout.txt` に simulation の出力が入ることを確認する。
5. 2 回連続実行して、ログディレクトリ名が衝突しないことを確認する。

## Phase 2: batch macro 実行を復活させる

### 目的

外側 script から `./sim macros/run.mac` のように実行できるようにする。

現在の `sim.cc` は `G4UIExecutive` を常に作成し、最後に `ui->SessionStart()` している。batch 実行用の分岐がコメントアウトされているため、これを復活させる。

### 変更候補ファイル

```text
sim.cc
```

### 実装方針

`argc == 1` のとき interactive mode、`argc > 1` のとき batch mode にする。

### 実装例

```cpp
G4UIExecutive* ui = nullptr;
if (argc == 1) {
    ui = new G4UIExecutive(argc, argv);
}
```

run manager と vis manager の初期化後、実行部分を次のようにする。

```cpp
G4UImanager* UImanager = G4UImanager::GetUIpointer();

if (ui) {
    ui->SessionStart();
    delete ui;
}
else {
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
}
```

### テスト

1. `./sim` で interactive mode が起動することを確認する。
2. `./sim macros/run.mac` で batch 実行されることを確認する。
3. batch 実行時に UI window 待ちにならないことを確認する。
4. `stdout.txt` に `/run/beamOn` の進行出力が保存されることを確認する。

## Phase 3: ROOT 出力パスを環境変数から受け取る

### 目的

外側 script が決めた `root/<run_label>.root` に ROOT ファイルを保存する。

### 変更候補ファイル

```text
src/RunAction.cc
```

### 追加 include 候補

```cpp
#include <cstdlib>
```

### 実装例

`BeginOfRunAction()` の ROOT file name 決定部分を切り出す。

```cpp
namespace {
G4String GetOutputRootFileName(const G4Run* run)
{
    const char* envOutput = std::getenv("G4_OUTPUT_ROOT");
    if (envOutput && envOutput[0] != '\0') {
        return G4String(envOutput);
    }

    std::stringstream strRunID;
    strRunID << run->GetRunID();
    return "../root/output" + strRunID.str() + ".root";
}
}
```

`BeginOfRunAction()` では次のように使う。

```cpp
void RunAction::BeginOfRunAction(const G4Run* run)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->OpenFile(GetOutputRootFileName(run));
}
```

### テスト

1. `G4_OUTPUT_ROOT` 未指定時に従来通り `../root/output0.root` が出ることを確認する。
2. `G4_OUTPUT_ROOT=/path/to/test.root` 指定時にその場所へ保存されることを確認する。
3. `run_with_log.sh` 経由で `root/<run_label>.root` が作られることを確認する。
4. `output.txt` に書いた ROOT path と実際の ROOT file path が一致することを確認する。

## Phase 4: seed と MT 情報を保存する

### 目的

再現性に最低限必要な乱数 seed と thread 数を保存する。

### 変更候補ファイル

```text
sim.cc
```

### 追加 include 候補

```cpp
#include <cstdlib>
#include <fstream>
#include <string>
```

### 補助関数の実装例

```cpp
namespace {
std::string GetEnvString(const char* name)
{
    const char* value = std::getenv(name);
    if (value && value[0] != '\0') {
        return std::string(value);
    }
    return "";
}

void WriteRandomLog(const std::string& logDir, unsigned long seed)
{
    if (logDir.empty()) return;

    std::ofstream ofs(logDir + "/random.txt");
    ofs << "RANDOM_ENGINE=CLHEP::MTwistEngine\n";
    ofs << "MASTER_SEED=" << seed << "\n";
}

void WriteMTLog(
    const std::string& logDir,
    const std::string& mode,
    G4int nThreads,
    G4int nCores,
    const std::string& policy
)
{
    if (logDir.empty()) return;

    std::ofstream ofs(logDir + "/mt.txt");
    ofs << "MODE=" << mode << "\n";
    ofs << "NUMBER_OF_THREADS=" << nThreads << "\n";
    ofs << "NUMBER_OF_CORES=" << nCores << "\n";
    ofs << "THREAD_POLICY=" << policy << "\n";
}
}
```

### `main()` 内の実装例

seed 生成後に保存する。

```cpp
std::string logDir = GetEnvString("G4_LOG_DIR");

std::random_device seed_gen;
auto seed = seed_gen();
G4Random::setTheEngine(new CLHEP::MTwistEngine);
G4cout << "Random Seed is " << seed << G4endl;
G4Random::setTheSeed(seed);

WriteRandomLog(logDir, seed);
```

MT thread 数決定後に保存する。

```cpp
#ifdef G4MULTITHREADED
    G4MTRunManager* runManager = new G4MTRunManager;

    G4int nCores = G4Threading::G4GetNumberOfCores();
    G4int nThreads = nCores - 4;
    if (nThreads < 1) {
        nThreads = 1;
    }

    runManager->SetNumberOfThreads(nThreads);
    WriteMTLog(logDir, "multithread", nThreads, nCores, "cores_minus_4_min_1");
#else
    G4RunManager* runManager = new G4RunManager;
    WriteMTLog(logDir, "singlethread", 1, 1, "fixed_1");
#endif
```

### テスト

1. `random.txt` が作成されることを確認する。
2. `stdout.txt` の seed と `random.txt` の seed が一致することを確認する。
3. `mt.txt` の thread 数が標準出力の thread 数と一致することを確認する。
4. core 数が 4 以下の環境でも `nThreads` が 1 未満にならないことを確認する。

## Phase 5: parameter.env を script 側で暫定保存する

### 目的

最初は C++ に大きく手を入れず、macro から読める情報と既定値を保存する。

### 変更候補ファイル

```text
scripts/run_with_log.sh
```

### 実装例

```bash
beam_on="$(awk '$1 == "/run/beamOn" { print $2 }' "${macro}" | tail -n 1)"
source_type="$(awk '$1 == "/mygen/sourceType" { print $2 }' "${macro}" | tail -n 1)"

if [ -z "${beam_on}" ]; then
    beam_on="unknown"
fi

if [ -z "${source_type}" ]; then
    source_type="neutron"
fi

case "${source_type}" in
    neutron)
        particle="neutron"
        energy_value="1.00"
        energy_unit="MeV"
        direction_mode="random_isotropic"
        ;;
    gamma)
        particle="gamma"
        energy_value="1.00"
        energy_unit="MeV"
        direction_mode="fixed_1_0_0"
        ;;
    "gamma(137Cs)")
        particle="gamma"
        energy_value="0.661660"
        energy_unit="MeV"
        direction_mode="random_isotropic"
        ;;
    "137Cs")
        particle="ion:Cs137"
        energy_value="0"
        energy_unit="eV"
        direction_mode="at_rest"
        ;;
    "90Sr")
        particle="ion:Sr90"
        energy_value="0"
        energy_unit="eV"
        direction_mode="at_rest"
        ;;
    "90Y")
        particle="ion:Y90"
        energy_value="0"
        energy_unit="eV"
        direction_mode="at_rest"
        ;;
    *)
        particle="unknown"
        energy_value="unknown"
        energy_unit="unknown"
        direction_mode="unknown"
        ;;
esac

{
    echo "SOURCE_TYPE=${source_type}"
    echo "PARTICLE=${particle}"
    echo "ENERGY_VALUE=${energy_value}"
    echo "ENERGY_UNIT=${energy_unit}"
    echo "NUMBER_OF_PARTICLES_PER_EVENT=1"
    echo "NUMBER_OF_EVENTS=${beam_on}"
    echo "POSITION=0 0 0 m"
    echo "DIRECTION_MODE=${direction_mode}"
    echo "MACRO_FILE=${macro}"
} > "${log_dir}/parameter.env"
```

### 注意点

この Phase の `parameter.env` は、あくまで `PrimaryGenerator.cc` の現状実装を script 側に写したもの。将来的に `PrimaryGenerator.cc` の値を変えたとき、script 側の case 文も更新しなければならない。

この重複をなくすために、次の Phase で `RunConfig` を導入する。

### テスト

1. `macros/run.mac` の `/run/beamOn 1000000` が保存されることを確認する。
2. `/mygen/sourceType gamma` を含む test macro で `SOURCE_TYPE=gamma` になることを確認する。
3. source type 未指定時に `SOURCE_TYPE=neutron` になることを確認する。
4. `PrimaryGenerator.cc` の各分岐と script の case 文が対応していることを目視確認する。

## Phase 6: RunConfig クラスを導入する

### 目的

`PrimaryGenerator` が使う条件と、ログに保存する条件を 1 箇所に集約する。

### 追加候補ファイル

```text
include/RunConfig.hh
src/RunConfig.cc
```

### `RunConfig.hh` 実装例

```cpp
#ifndef RUNCONFIG_HH
#define RUNCONFIG_HH

#include "globals.hh"
#include "G4ThreeVector.hh"

class RunConfig {
public:
    enum class DirectionMode {
        RandomIsotropic,
        Fixed,
        AtRest
    };

    RunConfig();

    void SetSourceType(const G4String& sourceType);

    const G4String& GetSourceType() const { return fSourceType; }
    const G4String& GetParticleName() const { return fParticleName; }
    G4double GetEnergy() const { return fEnergy; }
    const G4String& GetEnergyUnitName() const { return fEnergyUnitName; }
    G4int GetParticlesPerEvent() const { return fParticlesPerEvent; }
    const G4ThreeVector& GetPosition() const { return fPosition; }
    const G4ThreeVector& GetFixedDirection() const { return fFixedDirection; }
    DirectionMode GetDirectionMode() const { return fDirectionMode; }

    G4String GetDirectionModeName() const;

private:
    G4String fSourceType = "neutron";
    G4String fParticleName = "neutron";
    G4double fEnergy = 1.00;
    G4String fEnergyUnitName = "MeV";
    G4int fParticlesPerEvent = 1;
    G4ThreeVector fPosition = G4ThreeVector(0., 0., 0.);
    G4ThreeVector fFixedDirection = G4ThreeVector(1., 0., 0.);
    DirectionMode fDirectionMode = DirectionMode::RandomIsotropic;
};

#endif
```

### `RunConfig.cc` 実装例

```cpp
#include "RunConfig.hh"
#include "G4SystemOfUnits.hh"

RunConfig::RunConfig()
{
    SetSourceType("neutron");
}

void RunConfig::SetSourceType(const G4String& sourceType)
{
    fSourceType = sourceType;
    fParticlesPerEvent = 1;
    fPosition = G4ThreeVector(0., 0., 0.);

    if (sourceType == "neutron") {
        fParticleName = "neutron";
        fEnergy = 1.00 * MeV;
        fEnergyUnitName = "MeV";
        fDirectionMode = DirectionMode::RandomIsotropic;
    }
    else if (sourceType == "gamma") {
        fParticleName = "gamma";
        fEnergy = 1.00 * MeV;
        fEnergyUnitName = "MeV";
        fFixedDirection = G4ThreeVector(1., 0., 0.);
        fDirectionMode = DirectionMode::Fixed;
    }
    else if (sourceType == "gamma(137Cs)") {
        fParticleName = "gamma";
        fEnergy = 0.661660 * MeV;
        fEnergyUnitName = "MeV";
        fDirectionMode = DirectionMode::RandomIsotropic;
    }
    else if (sourceType == "137Cs") {
        fParticleName = "ion:Cs137";
        fEnergy = 0. * eV;
        fEnergyUnitName = "eV";
        fDirectionMode = DirectionMode::AtRest;
    }
    else if (sourceType == "90Sr") {
        fParticleName = "ion:Sr90";
        fEnergy = 0. * eV;
        fEnergyUnitName = "eV";
        fDirectionMode = DirectionMode::AtRest;
    }
    else if (sourceType == "90Y") {
        fParticleName = "ion:Y90";
        fEnergy = 0. * eV;
        fEnergyUnitName = "eV";
        fDirectionMode = DirectionMode::AtRest;
    }
}

G4String RunConfig::GetDirectionModeName() const
{
    if (fDirectionMode == DirectionMode::RandomIsotropic) {
        return "random_isotropic";
    }
    if (fDirectionMode == DirectionMode::Fixed) {
        return "fixed";
    }
    return "at_rest";
}
```

### テスト

1. `RunConfig config;` の default が `neutron` になることを確認する。
2. `SetSourceType("gamma")` 後に particle が `gamma`、energy が `1 MeV` になることを確認する。
3. `SetSourceType("gamma(137Cs)")` 後に energy が `0.661660 MeV` になることを確認する。
4. 未対応 source type を渡した場合の扱いを決める。

未対応 source type については、最初は `G4Exception` で止める設計が安全。

## Phase 7: PrimaryGenerator を RunConfig 参照型にする

### 目的

粒子発生条件を `PrimaryGenerator` 内に直接書く状態から、`RunConfig` を参照する状態へ移行する。

### 変更候補ファイル

```text
include/PrimaryGenerator.hh
src/PrimaryGenerator.cc
include/PrimaryGeneratorMessenger.hh
src/PrimaryGeneratorMessenger.cc
```

### `PrimaryGenerator.hh` 変更例

```cpp
class RunConfig;

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGenerator(RunConfig* runConfig);
    ~PrimaryGenerator();

    void GeneratePrimaries(G4Event*) override;

    void SetSourceType(G4String type);

private:
    G4ParticleGun* fParticleGun = nullptr;
    PrimaryGeneratorMessenger* fMessenger = nullptr;
    RunConfig* fRunConfig = nullptr;
};
```

### `PrimaryGenerator.cc` 変更例

```cpp
PrimaryGenerator::PrimaryGenerator(RunConfig* runConfig)
    : fRunConfig(runConfig)
{
    fParticleGun = new G4ParticleGun(1);
    fMessenger = new PrimaryGeneratorMessenger(this);

    fParticleGun->SetParticlePosition(fRunConfig->GetPosition());
}

void PrimaryGenerator::SetSourceType(G4String type)
{
    fRunConfig->SetSourceType(type);
}
```

`GeneratePrimaries()` では `RunConfig` の値を使う。

```cpp
void PrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
    const auto& sourceType = fRunConfig->GetSourceType();

    if (sourceType == "137Cs") {
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion = ionTable->GetIon(55, 137, 0.0);
        fParticleGun->SetParticleDefinition(ion);
    }
    else if (sourceType == "90Sr") {
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion = ionTable->GetIon(38, 90, 0.0);
        fParticleGun->SetParticleDefinition(ion);
    }
    else if (sourceType == "90Y") {
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion = ionTable->GetIon(39, 90, 0.0);
        fParticleGun->SetParticleDefinition(ion);
    }
    else {
        G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition* particle =
            particleTable->FindParticle(fRunConfig->GetParticleName());
        fParticleGun->SetParticleDefinition(particle);
    }

    fParticleGun->SetParticleEnergy(fRunConfig->GetEnergy());
    fParticleGun->SetParticlePosition(fRunConfig->GetPosition());

    if (fRunConfig->GetDirectionMode() == RunConfig::DirectionMode::RandomIsotropic) {
        fParticleGun->SetParticleMomentumDirection(G4RandomDirection());
    }
    else if (fRunConfig->GetDirectionMode() == RunConfig::DirectionMode::Fixed) {
        fParticleGun->SetParticleMomentumDirection(fRunConfig->GetFixedDirection());
    }
    else {
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 0.));
    }

    fParticleGun->GeneratePrimaryVertex(anEvent);
}
```

### テスト

1. 既存の `neutron` run で挙動が変わらないことを確認する。
2. `/mygen/sourceType gamma` が従来通り動くことを確認する。
3. `/mygen/sourceType gamma(137Cs)` が従来通り動くことを確認する。
4. `parameter.env` に保存される条件と実際の発生条件が一致することを確認する。

## Phase 8: ActionInitialization が RunConfig を所有する

### 目的

`PrimaryGenerator` と `RunAction` が同じ `RunConfig` を参照できるようにする。

### 変更候補ファイル

```text
include/ActionInitialization.hh
src/ActionInitialization.cc
```

### 設計案

`ActionInitialization` が `RunConfig` を所有する。

```cpp
#include "RunConfig.hh"

class ActionInitialization : public G4VUserActionInitialization
{
public:
    ActionInitialization();
    ~ActionInitialization();

    void BuildForMaster() const override;
    void Build() const override;

private:
    RunConfig* fRunConfig = nullptr;
};
```

ただし `Build()` は thread ごとに呼ばれるため、ポインタ共有の所有権に注意する。最初は `ActionInitialization` のコンストラクタで `RunConfig` を作り、destructor で delete する案が分かりやすい。

```cpp
ActionInitialization::ActionInitialization()
{
    fRunConfig = new RunConfig();
}

ActionInitialization::~ActionInitialization()
{
    delete fRunConfig;
}
```

worker 側では:

```cpp
PrimaryGenerator* generator = new PrimaryGenerator(fRunConfig);
SetUserAction(generator);

RunAction* runAction = new RunAction(eventAction, fRunConfig);
SetUserAction(runAction);
```

master 側では:

```cpp
RunAction* runAction = new RunAction(masterEventAction, fRunConfig);
SetUserAction(runAction);
```

### 注意点

MT mode では `RunConfig` を複数 thread から読む可能性がある。run 開始後に `RunConfig` を変更しない設計にする。`/mygen/sourceType` は `G4State_PreInit` または `G4State_Idle` のみ許可されているので、`beamOn` 中に変更しない方針と相性がよい。

### テスト

1. `ActionInitialization` の build が通ることを確認する。
2. worker 側 `PrimaryGenerator` が `RunConfig` を参照できることを確認する。
3. master 側 `RunAction` が `RunConfig` を参照できることを確認する。
4. MT run で source type が全 worker に反映されることを確認する。

## Phase 9: RunLogger クラスを導入する

### 目的

ログ出力処理を `RunAction` や `sim.cc` に直接増やしすぎないようにする。

### 追加候補ファイル

```text
include/RunLogger.hh
src/RunLogger.cc
```

### `RunLogger.hh` 実装例

```cpp
#ifndef RUNLOGGER_HH
#define RUNLOGGER_HH

#include "globals.hh"

class G4Run;
class RunConfig;

class RunLogger {
public:
    RunLogger();

    void BeginRun(const G4Run* run, const RunConfig& config);
    void EndRun(const G4Run* run);

    const G4String& GetLogDir() const { return fLogDir; }
    const G4String& GetRootFile() const { return fRootFile; }

private:
    G4String fRunLabel;
    G4String fLogDir;
    G4String fRootFile;

    void LoadFromEnvironment();
    void WriteParameterEnv(const G4Run* run, const RunConfig& config) const;
    void WriteOutputStart(const G4Run* run) const;
    void WriteOutputEnd(const G4Run* run) const;
};

#endif
```

### `RunLogger.cc` 実装例

```cpp
#include "RunLogger.hh"
#include "RunConfig.hh"

#include "G4Run.hh"

#include <cstdlib>
#include <fstream>

namespace {
G4String GetEnvOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    if (value && value[0] != '\0') {
        return G4String(value);
    }
    return "";
}
}

RunLogger::RunLogger()
{
    LoadFromEnvironment();
}

void RunLogger::LoadFromEnvironment()
{
    fRunLabel = GetEnvOrEmpty("G4_RUN_LABEL");
    fLogDir = GetEnvOrEmpty("G4_LOG_DIR");
    fRootFile = GetEnvOrEmpty("G4_OUTPUT_ROOT");
}

void RunLogger::BeginRun(const G4Run* run, const RunConfig& config)
{
    if (fLogDir.empty()) return;

    WriteParameterEnv(run, config);
    WriteOutputStart(run);
}

void RunLogger::EndRun(const G4Run* run)
{
    if (fLogDir.empty()) return;

    WriteOutputEnd(run);
}

void RunLogger::WriteParameterEnv(const G4Run* run, const RunConfig& config) const
{
    std::ofstream ofs(fLogDir + "/parameter.env");

    ofs << "RUN_ID=" << run->GetRunID() << "\n";
    ofs << "NUMBER_OF_EVENTS=" << run->GetNumberOfEventToBeProcessed() << "\n";
    ofs << "SOURCE_TYPE=" << config.GetSourceType() << "\n";
    ofs << "PARTICLE=" << config.GetParticleName() << "\n";
    ofs << "ENERGY_INTERNAL_VALUE=" << config.GetEnergy() << "\n";
    ofs << "ENERGY_UNIT_NAME=" << config.GetEnergyUnitName() << "\n";
    ofs << "NUMBER_OF_PARTICLES_PER_EVENT=" << config.GetParticlesPerEvent() << "\n";
    ofs << "DIRECTION_MODE=" << config.GetDirectionModeName() << "\n";
}

void RunLogger::WriteOutputStart(const G4Run* run) const
{
    std::ofstream ofs(fLogDir + "/output.txt", std::ios::app);

    ofs << "RUN_ID=" << run->GetRunID() << "\n";
    ofs << "ROOT_FILE=" << fRootFile << "\n";
    ofs << "LOG_DIR=" << fLogDir << "\n";
}

void RunLogger::WriteOutputEnd(const G4Run* run) const
{
    std::ofstream ofs(fLogDir + "/output.txt", std::ios::app);

    ofs << "NUMBER_OF_EVENTS_PROCESSED=" << run->GetNumberOfEvent() << "\n";
}
```

### 注意点

`GetNumberOfEventToBeProcessed()` が環境や Geant4 version によって使いづらい場合は、Phase 5 のように script 側で macro から `/run/beamOn` を読む方式を残す。

### テスト

1. `G4_LOG_DIR` 未指定時に何も書かず、simulation が通常通り動くことを確認する。
2. `G4_LOG_DIR` 指定時に `parameter.env` が作られることを確認する。
3. `parameter.env` の source type が `/mygen/sourceType` と一致することを確認する。
4. `output.txt` に run 終了後の event 数が追記されることを確認する。

## Phase 10: RunAction に RunLogger を接続する

### 目的

run 開始時・終了時に C++ 側の実行条件を保存する。

### 変更候補ファイル

```text
include/RunAction.hh
src/RunAction.cc
```

### `RunAction.hh` 変更例

```cpp
#include "RunLogger.hh"

class RunConfig;

class RunAction : public G4UserRunAction {
public:
    RunAction(EventAction* eventAction, RunConfig* runConfig);
    ~RunAction();

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

private:
    EventAction* fEventAction = nullptr;
    RunConfig* fRunConfig = nullptr;
    AnalysisOutput fAnalysisOutput;
    RunLogger fRunLogger;
};
```

### `RunAction.cc` 変更例

```cpp
RunAction::RunAction(EventAction* eventAction, RunConfig* runConfig)
    : fEventAction(eventAction),
      fRunConfig(runConfig)
{
    fAnalysisOutput.Book(fEventAction);
    fEventAction->SetAnalysisOutput(&fAnalysisOutput);
}
```

`BeginOfRunAction()` では、ログ書き込みは master のみに限定する。

```cpp
void RunAction::BeginOfRunAction(const G4Run* run)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->OpenFile(GetOutputRootFileName(run));

    if (IsMaster() && fRunConfig) {
        fRunLogger.BeginRun(run, *fRunConfig);
    }
}
```

`EndOfRunAction()` も同様。

```cpp
void RunAction::EndOfRunAction(const G4Run* run)
{
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->Write();
    analysisManager->CloseFile();

    if (IsMaster()) {
        fRunLogger.EndRun(run);
    }

    G4cout << "Finishing run " << run->GetRunID() << G4endl;
}
```

### 重要な注意点

`analysisManager->OpenFile()`, `Write()`, `CloseFile()` を `IsMaster()` で単純に囲むかどうかは慎重に確認する。現在の `AnalysisOutput::Book()` では `SetNtupleMerging(true)` を使っているため、Geant4 analysis manager の MT merge の流れを壊さないことを優先する。

まずは ROOT 出力処理は既存と同じ場所で呼び、追加ログだけを `IsMaster()` で限定する。

### テスト

1. single thread で ROOT とログが両方作られることを確認する。
2. MT mode で ROOT とログが両方作られることを確認する。
3. `parameter.env` が worker 数ぶん重複して作られないことを確認する。
4. ROOT tree の event 数が期待値と一致することを確認する。

## Phase 11: version.txt を C++ または script で拡張する

### 目的

解析時に、どの環境で作られた ROOT かを追跡できるようにする。

### script 側で保存する項目

```bash
{
    echo "GIT_COMMIT=$(git -C "${project_dir}" rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "GIT_BRANCH=$(git -C "${project_dir}" branch --show-current 2>/dev/null || echo unknown)"
    echo "CXX_COMPILER=$(${CXX:-c++} --version | head -n 1 2>/dev/null || echo unknown)"
    echo
    echo "[git status --short]"
    git -C "${project_dir}" status --short 2>/dev/null || true
} > "${log_dir}/version.txt"
```

### C++ 側で追加できる項目

```cpp
#include "G4Version.hh"
```

```cpp
ofs << "GEANT4_VERSION=" << G4Version << "\n";
```

### テスト

1. `version.txt` に Git commit が入ることを確認する。
2. dirty worktree のとき `git status --short` に変更ファイルが出ることを確認する。
3. `git.diff` が作成されることを確認する。
4. Geant4 version が保存されることを確認する。

## Phase 12: physics.txt を保存する

### 目的

物理過程の設定差分を追跡できるようにする。

現在の `sim.cc` では、次の設定が行われている。

```cpp
G4HadronicParameters::Instance()
    ->SetTimeThresholdForRadioactiveDecay(1.0e+60 * CLHEP::year);
```

これは結果に影響するため、ログに残す。

### 実装候補

最初は `sim.cc` または `RunLogger` から固定文字列として保存する。

```cpp
void WritePhysicsLog(const std::string& logDir)
{
    if (logDir.empty()) return;

    std::ofstream ofs(logDir + "/physics.txt");
    ofs << "PHYSICS_LIST=PhysicsList\n";
    ofs << "RADIOACTIVE_DECAY_TIME_THRESHOLD=1.0e+60 year\n";
}
```

将来的には `PhysicsList` に summary を返す関数を追加する。

```cpp
G4String PhysicsList::GetSummary() const;
```

### テスト

1. `physics.txt` が作成されることを確認する。
2. radioactive decay threshold が保存されることを確認する。
3. `PhysicsList` の設定変更時に `physics.txt` も更新される運用にする。

## Phase 13: analysis_schema.txt を保存する

### 目的

ROOT tree の column 構造をログとして残す。

現在の column は `AnalysisOutput::Book()` に集約されているため、ここに schema 出力を追加しやすい。

### 変更候補ファイル

```text
include/AnalysisOutput.hh
src/AnalysisOutput.cc
```

### 実装案

`AnalysisOutput` に column 定義を返す関数を追加する。

```cpp
std::vector<G4String> GetColumnSchemaLines() const;
```

実装例:

```cpp
std::vector<G4String> AnalysisOutput::GetColumnSchemaLines() const
{
    return {
        "Scinti_Edep double",
        "Scinti_Evis double",
        "Scinti_Photons double",
        "Scinti_InteractionCount int",
        "Scinti_HitPos_Global vector<double>",
        "Scinti_HitPos_Local vector<double>",
        "Scinti_HitPos_Radius double",
        "PMT_SUM_Photons int",
        "PMT_SUM_Efficiency double",
        "PMT1_Photons int",
        "PMT1_Efficiency double",
        "PMT2_Photons int",
        "PMT2_Efficiency double",
        "PMT1_HitTimes vector<double>",
        "PMT1_HitPos_X vector<double>",
        "PMT1_HitPos_Y vector<double>",
        "PMT1_HitPos_Z vector<double>",
        "PMT2_HitTimes vector<double>",
        "PMT2_HitPos_X vector<double>",
        "PMT2_HitPos_Y vector<double>",
        "PMT2_HitPos_Z vector<double>"
    };
}
```

`RunLogger` に渡して保存する。

```cpp
void RunLogger::WriteAnalysisSchema(
    const std::vector<G4String>& schemaLines
) const
{
    if (fLogDir.empty()) return;

    std::ofstream ofs(fLogDir + "/analysis_schema.txt");
    for (const auto& line : schemaLines) {
        ofs << line << "\n";
    }
}
```

### テスト

1. `analysis_schema.txt` が作成されることを確認する。
2. `AnalysisOutput.cc` の column 定義と一致することを確認する。
3. ROOT file を開いて tree branch 名と比較する。

## Phase 14: Geant4 random store を検討する

### 目的

seed 値だけでなく、Geant4 が持つ乱数状態の保存も利用できるようにする。

### 実装候補

`RunAction::BeginOfRunAction()` に追加する。

```cpp
#include "G4RunManager.hh"
```

```cpp
G4RunManager::GetRunManager()->SetRandomNumberStore(true);
```

### 注意点

MT mode でどのファイルがどこに出るか、保存量が大きくなりすぎないかを確認する。最初から常時有効にせず、環境変数や macro command で切り替える設計がよい。

例:

```cpp
const char* randomStore = std::getenv("G4_RANDOM_STORE");
if (randomStore && G4String(randomStore) == "1") {
    G4RunManager::GetRunManager()->SetRandomNumberStore(true);
}
```

### テスト

1. `G4_RANDOM_STORE=0` で従来通り動くことを確認する。
2. `G4_RANDOM_STORE=1` で乱数状態ファイルが保存されることを確認する。
3. MT mode で保存ファイル数と保存場所を確認する。
4. 短い run で再現実行できるか確認する。

## Phase 15: 再現実行モードを作る

### 目的

過去の `logs/<run_label>/` を指定して、同じ条件で再実行できるようにする。

### 追加候補 script

```text
scripts/rerun_from_log.sh
```

### 実装方針

1. 指定された log directory から `parameter.env` を読む。
2. `random.txt` から seed を読む。
3. `macro.mac` を再利用する。
4. thread 数を同じにするか、現在環境に合わせるか選ぶ。
5. 新しい run label を作る。
6. `manifest.json` に `rerun_of` を記録する。

### script 実装イメージ

```bash
#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <executable> <old_log_dir>"
    exit 1
fi

exe="$1"
old_log_dir="$2"
macro="${old_log_dir}/macro.mac"

if [ ! -f "${macro}" ]; then
    echo "macro.mac not found in ${old_log_dir}"
    exit 1
fi

seed="$(awk -F= '$1 == "MASTER_SEED" { print $2 }' "${old_log_dir}/random.txt")"

export G4_FIXED_SEED="${seed}"

scripts/run_with_log.sh "${exe}" "${macro}"
```

この場合、`sim.cc` 側では `G4_FIXED_SEED` が指定されていれば `random_device` ではなくその seed を使う。

### `sim.cc` の seed 決定例

```cpp
unsigned long seed = 0;
const char* fixedSeed = std::getenv("G4_FIXED_SEED");

if (fixedSeed && fixedSeed[0] != '\0') {
    seed = std::stoul(fixedSeed);
}
else {
    std::random_device seed_gen;
    seed = seed_gen();
}

G4Random::setTheSeed(seed);
```

### テスト

1. 過去 log directory を指定して再実行できることを確認する。
2. `random.txt` の seed が再利用されることを確認する。
3. 同じ seed / 同じ thread 数 / 同じ macro で短い run の結果が再現することを確認する。
4. rerun の `manifest.json` に元 run label が残ることを確認する。

## 実装順序まとめ

1. `scripts/run_with_log.sh` を作る。
2. `sim.cc` の batch 実行を復活させる。
3. `RunAction.cc` で `G4_OUTPUT_ROOT` を読む。
4. `sim.cc` で `random.txt` と `mt.txt` を保存する。
5. script 側で暫定 `parameter.env` を保存する。
6. `RunConfig` を追加する。
7. `PrimaryGenerator` を `RunConfig` 参照に変更する。
8. `ActionInitialization` から `RunConfig` を渡す。
9. `RunLogger` を追加する。
10. `RunAction` から `RunLogger` を呼ぶ。
11. `version.txt`, `physics.txt`, `analysis_schema.txt` を拡張する。
12. `G4_RANDOM_STORE` による乱数状態保存を検討する。
13. `scripts/rerun_from_log.sh` を作る。

## 最初の最小実装

最初に実装すべき範囲は次の 4 つ。

```text
scripts/run_with_log.sh
sim.cc batch 実行対応
RunAction.cc の G4_OUTPUT_ROOT 対応
sim.cc の random.txt / mt.txt 出力
```

この段階で、以下が満たされる。

```text
root/<run_label>.root
logs/<run_label>/macro.mac
logs/<run_label>/version.txt
logs/<run_label>/git.diff
logs/<run_label>/random.txt
logs/<run_label>/mt.txt
logs/<run_label>/stdout.txt
logs/<run_label>/output.txt
```

その後、`RunConfig` と `RunLogger` を導入して、ログ内容を C++ 内部の実際の実行条件と一致させる。

## 開発時の確認チェックリスト

各 Phase ごとに、必ず短い event 数で確認する。

```text
/run/beamOn 10
```

確認項目:

- simulation が最後まで終了する。
- ROOT ファイルが作成される。
- log directory が作成される。
- ROOT ファイル名と log directory 名の run label が一致する。
- `stdout.txt` に seed と thread 数が出ている。
- `random.txt` の seed が stdout と一致する。
- `mt.txt` の thread 数が stdout と一致する。
- `parameter.env` の source type と macro の指定が一致する。
- MT mode でログファイルが worker 数ぶん重複生成されない。
- 連続 2 回実行して、前回の ROOT と log が上書きされない。
```
