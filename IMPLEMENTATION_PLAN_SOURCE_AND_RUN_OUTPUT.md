# 線源設定・複数 Run・出力管理の実装計画

作成日: 2026-09-25

本書は実装計画であり、コードの変更・build・test は行わない。現行コードと Geant4 11.2.2 のローカルソースについて、会話中に確認した内容を基にする。既存の未コミット変更を前提とし、実装時には最新の状態との差分を再確認する。

## 1. 目的と採用方針

- 入射粒子の条件を Geant4 の標準 GPS コマンドでマクロに明記する。
- 同一プロセス内でエネルギーを変更し、複数の Run を実行できるようにする。
- 実行入力と、各 Run に実際に採用された線源条件を保存する。
- 時刻を用いる命名方式と、線源情報を用いる命名方式を shell のオプションで選択する。
- 各 Run の ROOT 出力、条件記録、実行状態を確実に対応づける。

基本単位は「一つの Run に一組の線源条件」とする。線源条件は Run 間で変更し、Run 中は固定する。初期実装は一つの GPS source、点線源、単色エネルギー、等方または固定方向を対象とする。中性子以外の現行線源も移行対象とする。

## 2. 現状と変更が必要な理由

- `RunConfig` が線源プリセットを保持し、`PrimaryGenerator` が各 event で ParticleGun の設定を上書きしている。マクロだけを `/gun/` に変更しても移行は成立しない。
- `RunConfig::GetParticlesPerEvent()` は生成処理に接続されていない。
- 各 worker の Messenger が一つの共有 RunConfig を変更する経路がある。
- `scripts/run_with_log.sh` は実行マクロをコピーするため、元マクロの後日の変更だけで入力が失われるわけではない。ただしコード内のプリセット値はマクロに含まれない。
- RunConfig の確定条件を直接記録する機能はない。
- `G4_OUTPUT_ROOT` が指定されると、現行 RunAction は全 Run に同じ出力名を返す。複数 Run の結果を安全に分離するには変更が必要。
- shell の `set -e` により、実行失敗後の終了情報の追記に進めない場合がある。
- 現行 AnalysisConfig は光学記録モード・中性子履歴対象を管理する。線源設定や命名規則をここへ混在させない。

## 3. 責務とデータの分離

以下の名称は実装時の候補であり、既存構成に合わせて調整してよい。

| 要素 | 責務 | 更新単位 |
|---|---|---|
| shell script | CLI 解釈、実行 ID と保存先の確保、入力保存、プロセス終了記録 | 実行ごと |
| OutputConfig | 命名方式、実行 ID、出力ディレクトリ | 起動時に固定 |
| PrimaryGenerator | G4GeneralParticleSource に一次粒子生成を委譲 | worker ごとに生成器を所有 |
| SourceConditionsReader | GPS の設定オブジェクトから線源条件を取得 | Run ごと |
| RunConditions | 採用した線源・解析設定の不変な記録 | Run ごと |
| RunOutputInfo | Run ID と確定出力パス | Run ごと |
| 出力名生成処理 | OutputConfig と RunConditions から名前を決定 | Run ごと |
| RunAction / 条件保存処理 | ROOT の開始・終了、条件・実行結果の保存 | Run ごと |

変更可能な線源設定の正本は GPS とする。RunConfig と GPS の二重管理を避け、RunConfig は廃止するか、設定を書き換えない記録用 RunConditions へ置き換える。

OutputConfig は main で一度構築し、ActionInitialization から必要な RunAction へ読み取り専用で渡す。共有ポインタを使う場合も、読み取り専用型にするだけで同期が成立するとは考えず、Run 中に書き換えない設計にする。

## 4. GPS への移行

対象: `include/PrimaryGenerator.hh`、`src/PrimaryGenerator.cc`、PrimaryGeneratorMessenger、RunConfig、ActionInitialization、各実行マクロ。

1. PrimaryGenerator の ParticleGun を G4GeneralParticleSource に置き換える。
2. GeneratePrimaries は GPS の GeneratePrimaryVertex を呼ぶ構成にする。
3. RunConfig による毎 event の設定上書きを除去する。
4. `/mygen/sourceType` の責務を GPS マクロへ移す。独自コマンドと GPS の同時運用による優先順位問題を残さない。
5. 全マクロと README の旧コマンド参照を更新した後、不要となった Messenger / RunConfig を整理する。

移行時に維持する現行条件:

| 現行指定 | 粒子・運動エネルギー | 方向 |
|---|---|---|
| neutron | neutron、0.5 MeV | 等方 |
| thermal_neutron | neutron、0.025 eV | 等方 |
| electron | e-、50 keV | 等方 |
| gamma | gamma、2.3 MeV | 等方 |
| gamma(137Cs) | gamma、0.661660 MeV | 等方 |
| 137Cs | Cs-137 イオン、0 eV | 静止 |
| 90Sr | Sr-90 イオン、0 eV | 静止 |
| 90Y | Y-90 イオン、0 eV | 静止 |

現行の発生位置は原点、粒子数は 1。放射性イオンは GPS の ion 指定へ移し、Z・A・励起エネルギー・電荷の扱いを確認する。単色 gamma と放射性核種そのものを混同しない。既存の放射性崩壊物理設定は維持する。

## 5. エネルギー走査マクロ

例: `scan.mac`

```text
/run/initialize
/gps/particle neutron
/gps/number 1
/gps/pos/type Point
/gps/pos/centre 0 0 0 mm
/gps/ang/type iso
/gps/ene/type Mono
/control/alias eventsPerRun 1000000
/control/foreach one_energy.mac energyMeV "0.1 0.2 0.5 1.0 2.0"
```

例: `one_energy.mac`

```text
/gps/ene/mono {energyMeV} MeV
/run/beamOn {eventsPerRun}
```

等間隔の走査には次も使用できる。

```text
/control/loop one_energy.mac energyMeV 0.1 1.0 0.1
```

エネルギー変更だけでは geometry / physics を再初期化しない。複数 Run を一つのプロセスで順次実行する。マクロの相対パスは入力保存先を基準に解決するなど、実行ディレクトリの規約を明示する。

## 6. 条件の取得と保存

マクロを独自に解析して最終設定を推測する方式は採用しない。Geant4 が解釈した GPS の分布設定から条件を取得する。生成後の GetParticleEnergy / Position / Direction などの値を、そのまま分布設定と見なさない。

初期スキーマに含める情報:

- schema version、実行 ID、Run ID。
- 粒子名・PDG コード。イオンの場合は Z・A・励起エネルギー・電荷。
- 一つの source が生成する粒子数。初期実装では単一 source のため event あたりの粒子数と一致する。
- エネルギー分布の種類、単色エネルギー、明示した単位。
- 位置分布の種類、中心座標と単位。
- 角度分布の種類、固定方向または等方分布の角度範囲・必要な座標系設定。
- OpticalRecordingMode、NeutronHistoryTarget。
- 要求 event 数、処理済み event 数、開始・終了状態。
- ROOT と条件ファイルのパス。

数値は記録用の単位を統一し、列名または単位フィールドで明示する。名前に表示する丸めた値とは別に、条件ファイルには精度を保持した値を保存する。

初期実装は Run ごとの JSON を正式な条件記録とする。既存 ROOT の RunID と対応づける。ROOT 単独での配布が必要になった段階で同じスナップショットから RunConditions tree を追加し、別々に条件を組み立てない。

未対応の複数 source や分布を使用した場合、単色設定として誤って記録しない。対応フィールドを追加するまでは、初期実装の記録保証範囲外として Run 開始前に明確にエラーを通知する。

## 7. マルチスレッドでの確定タイミング

ここは実装前に Geant4 11.2.2 のソースと example で確定すべき主要事項。

- `/gps/` コマンド適用後、当該 Run のイベント生成前に条件を取得する。
- master の BeginOfRunAction が worker の PrimaryGenerator を直接参照する前提を置かない。
- GPS の共有設定データへのアクセス方法とコマンド反映時点を確認し、master が確定設定を読み取れる経路を優先する。記録目的だけで GPS を追加生成し、コマンド登録や source 状態を増やさない。
- worker から収集する必要がある場合は、同期と集約手順を明示する。最初に来た worker の値を無条件で正解にしない。
- ファイルを開く前に、その Run の条件・出力名の整合性を保証する。
- master / worker は同じ実行 ID・Run ID・出力名規則を使用する。worker ごとに現在時刻を取得しない。
- JSON と実行一覧は一つの担当が保存し、複数 worker による競合書き込みを避ける。

この経路が確認できるまでは、BeginOfRunAction に単純な getter を追加するだけで完成と判断しない。

## 8. 命名方式と shell / Geant4 間の受け渡し

CLI 案:

```bash
./run_with_log.sh --naming timestamp ./sim scan.mac
./run_with_log.sh --naming source ./sim scan.mac
```

既存の二つの位置引数だけで実行した場合は timestamp を既定とし、操作の互換性を保つ。

環境変数案:

| 変数 | 内容 |
|---|---|
| G4_OUTPUT_DIR | 実行単位の保存先 |
| G4_OUTPUT_NAMING | timestamp または source |
| G4_EXECUTION_ID | shell が一度だけ生成した一意な実行 ID |
| G4_LOG_DIR | ログ保存先。既存の乱数・並列設定ログにも使用 |

実行 ID は日時に衝突回避用の識別子を加え、ディレクトリ作成時にも重複を検出する。shell を経由しない直接実行では、main が同じ規約の既定値を一度だけ生成する。

名前の例:

```text
timestamp: 20260925_153000_xxxx_run000.root
source:    neutron_mono_0p5MeV_run000.root
```

両方式とも Run ID を必須とする。別実行との区別は実行 ID 付きディレクトリで保証する。粒子名のファイル名向け変換、単位、数値表記、ロケール非依存の規約を出力名生成処理に集約する。ファイル名は識別の補助であり、完全な条件は JSON に保存する。

G4_OUTPUT_ROOT は固定された完成ファイル名として使い続けない。移行期間は明示された旧値を Run ID 付きベース名として扱い、新方式と同時指定された場合は設定エラーにするなど、曖昧さのない移行規則を決めて文書化する。

## 9. 出力の構造とライフサイクル

```text
results/<execution_id>/
  inputs/
    scan.mac
    one_energy.mac
  logs/
    stdout.txt
    random.txt
    mt.txt
    version.txt
    git.diff
  manifest.json
  run_000.root       # 実際の basename は選択した命名方式による
  run_000.json
  run_001.root
  run_001.json
```

- 1 Run ごとに一つの ROOT と同じ basename の JSON を作る。
- Run 開始時に条件と running 状態を保存し、終了時に処理件数と状態を更新する。
- JSON は一時ファイルからの置換などで、中途半端な更新を避ける。
- ROOT は Run ごとに OpenFile → Write → CloseFile を行う。既存の ntuple 定義と worker 出力のマージを維持する。
- 出力名の衝突を検出したら既存結果を上書きせず終了する。
- manifest は全 Run の ID、条件ファイル、ROOT パス、状態を一覧にする。線源条件を shell で再推測しない。
- manifest の更新担当を一つに限定する。例えば Geant4 が Run 一覧を管理し、shell は別の実行終了記録を残すことで競合を避ける。

要求 event 数と実際に処理した event 数を区別する。異常終了で終了処理が走らなければ、開始時記録を残して未完了と判別できるようにする。プロセス終了コードと各 Run の正常終了を同一視しない。

## 10. 入力・再現情報の保存

- 主マクロだけでなく、呼び出すマクロと外部スペクトルファイルも保存する。
- 初期実装では入力を一つのディレクトリにまとめてコピーし、その保存済み入力から実行する方式を優先する。マクロの任意の構文を shell で解析して依存関係を発見する仕組みは作らない。
- 保存ディレクトリ外の入力を使う場合は明示的な依存ファイル一覧を要求するなど、保存範囲を明確にする。
- 日時、実行コマンド、Geant4 バージョン、Git 情報、乱数エンジン・master seed、スレッド数を保存する。
- 現行 git diff は未追跡ファイルの内容を含まないことを明記する。完全なコード再現が必要ならコミット済み状態またはソーススナップショットを用いる。
- 同一プロセス内の後続 Run は、それ以前の実行も含む乱数状態に依存する。初期仕様は実行全体の再現を対象とし、master seed だけで任意の Run を単独再現できるとは保証しない。Run 単位の再現は Geant4 の乱数状態保存を検討する別項目とする。
- shell の失敗時記録は終了コードを明示的に捕捉して実装する。Geant4 のマクロ実行エラーがプロセス終了コードに反映されるかも sim.cc で確認する。

## 11. 実装順序と完了条件

### 段階 1: 契約の確定

OutputConfig、RunConditions の初期スキーマ、命名規則、単一 source の対応範囲、旧環境変数の扱いを決める。Geant4 11.2.2 における MT の条件取得経路を確認する。

完了条件: 条件の正本・取得時点・記録担当・出力名決定担当が明確。

### 段階 2: GPS による単一 Run

PrimaryGenerator と全線源マクロを移行する。旧プリセット上書きを除去する。

完了条件: 現行線源をマクロだけで表現でき、粒子数・単位・方向が意図通り。

### 段階 3: Run ごとの条件記録と出力分離

確定条件の取得、Run ID 付き ROOT、同じ basename の JSON を接続する。

完了条件: 二つの異なるエネルギーの Run が独立した出力と正しい条件記録を持つ。

### 段階 4: 命名方式と shell の整備

CLI・環境変数・OutputConfig を接続する。保存済み入力からの実行、manifest、失敗時記録を整える。

完了条件: 二つの命名方式で同じ物理条件が実行され、結果とログの対応が崩れない。

### 段階 5: 走査と既存機能の確認

foreach / loop のマクロを追加し、README に実行方法・保存仕様・対応範囲を記す。

完了条件: 複数 Run、単一・複数 worker、旧線源の移行、既存の検出器出力について下記確認が済む。

## 12. 実装後の確認項目

以下は実装担当者が行う確認計画であり、本計画作成時に実行するものではない。

- 少数 event の単一 Run: 粒子種、エネルギー、位置、角度設定、粒子数が記録と生成内容に一致。
- 0.1 / 0.5 / 1.0 MeV の連続 Run: 三つの ROOT と条件記録が作られ、過去の値が残らない。
- 同じエネルギーの再実行: 異なる Run ID で保存される。
- 1 MeV と 1000 keV: 内部の条件表現が一致する。
- summary / detailed: 既存 ROOT の列と光学記録動作を維持する。
- 単一 worker と複数 worker: 出力名・条件の一致、条件記録の重複なし、正常な ROOT マージ。
- master seed を保存した実行全体の再実行: 定めた環境条件の範囲で再現を確認。
- 元マクロの変更: 保存済み入力と条件 JSON が影響を受けない。
- 不正なマクロ・未対応分布・無効な保存先: 明確なエラーとなり、正常完了と記録されない。
- 途中終了: 完了済み Run の結果が残り、未完了 Run を判別できる。
- 同時起動: 保存先が衝突しない。
- shell を経由しない実行: 文書化した既定値で動作する。
- gamma(137Cs) と Cs-137 イオン: 別の線源として移行・記録される。

## 13. 参考資料

- GPS の最小生成器: `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/eventgenerator/exgps/src/PrimaryGeneratorAction.cc`
- 粒子・位置・等方放出・エネルギーのマクロ例: `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/eventgenerator/exgps/macros/test01.mac`
- loop の利用例: `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/runAndEvent/RE03/run1.mac`
- loop / foreach の構文確認: `/home/hashizume/Geant4/geant4-11.2.2/source/intercoms/src/G4UIcontrolMessenger.cc`
- [Geant4 GPS 公式資料](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/GettingStarted/generalParticleSource.html)
- [Geant4 解析ファイル管理の公式資料](https://geant4.web.cern.ch/documentation/pipelines/master/bfad_html/ForApplicationDevelopers/Analysis/managers.html)

オンライン資料は会話中に参照したもの。実装 API と MT の挙動は、使用中のローカル Geant4 11.2.2 を基準に確認する。
