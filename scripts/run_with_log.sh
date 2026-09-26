#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--naming timestamp|source] [--input file ...] <executable> <macro>"
    echo "Repeat --input for each additional input file."
}

fail() {
    echo "Error: $*" >&2
    exit 1
}

# ------------------------------------------------------------
# 引数
# ------------------------------------------------------------

naming="timestamp"

additional_inputs=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --naming)
            [[ $# -ge 2 ]] || fail "--naming requires a value."
            naming="$2"
            shift 2
            ;;
        --input)
            [[ $# -ge 2 ]] || fail "--input requires a file."
            additional_inputs+=("$2")
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            fail "Unknown option: $1"
            ;;
        *)
            break
            ;;
    esac
done

[[ $# -eq 2 ]] || {
    usage >&2
    exit 1
}

case "$naming" in
    timestamp|source)
        ;;
    *)
        fail "Naming mode must be timestamp or source."
        ;;
esac

exe="$(realpath -e -- "$1")"
macro="$(realpath -e -- "$2")"

[[ -f "$exe" && -x "$exe" ]] ||
    fail "Executable is not a runnable file: $exe"

[[ -f "$macro" && -r "$macro" ]] ||
    fail "Macro is not a readable file: $macro"

input_dir="$(dirname -- "$macro")"
macro_name="$(basename -- "$macro")"

# 主マクロは必ず保存する
input_sources=("$macro")

# 追加ファイルは主マクロと同じディレクトリ、
# またはその下のディレクトリに置く
for requested_input in "${additional_inputs[@]}"; do
    input_file="$(realpath -e -- "$requested_input")"

    [[ -f "$input_file" && -r "$input_file" ]] ||
        fail "Additional input is not a readable file: $input_file"

    case "$input_file" in
        "${input_dir}/"*)
            ;;
        *)
            fail "Additional input must be inside: $input_dir"
            ;;
    esac

    input_sources+=("$input_file")
done

# Geant4 のコマンドへ渡すマクロ名を単純な名前に限定する
case "$macro_name" in
    *[!a-zA-Z0-9_.-]*)
        fail "Macro filename must contain only letters, digits, _, -, or ."
        ;;
esac

project_dir="$(
    cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
    pwd -P
)"

manifest_writer="${project_dir}/scripts/write_manifest.py"

command -v python3 >/dev/null 2>&1 ||
    fail "python3 is required."

[[ -f "$manifest_writer" && -r "$manifest_writer" ]] ||
    fail "Manifest writer is not readable: $manifest_writer"

results_dir="${project_dir}/results"
mkdir -p -- "$results_dir"
results_dir="$(realpath -e -- "$results_dir")"


# ------------------------------------------------------------
# 実行 ID と保存先
# 命名方式に関係なく、同じ方法で実行 ID を生成する
# ------------------------------------------------------------

# ローカル時刻を秒まで使用する
timestamp="$(LC_ALL=C date +%Y%m%d_%H%M%S)"

execution_dir=""

# 最初は日時だけ。
# 同じ名前が存在する場合に限り、_001 ～ _999 を付ける。
for ((sequence = 0; sequence <= 999; sequence++)); do
    if [[ "$sequence" -eq 0 ]]; then
        candidate_id="$timestamp"
    else
        printf -v candidate_id \
            '%s_%03d' "$timestamp" "$sequence"
    fi

    candidate_dir="${results_dir}/${candidate_id}"

    # 存在確認と作成を分離せず、mkdir の成功で保存先を確保する
    if mkdir_message="$(mkdir -- "$candidate_dir" 2>&1)"; then
        execution_dir="$candidate_dir"
        execution_id="$candidate_id"
        break
    fi

    # 名前が使われている場合は、次の連番を試す
    if [[ -e "$candidate_dir" || -L "$candidate_dir" ]]; then
        continue
    fi

    # 権限不足など、名前の重複以外の失敗
    fail "Cannot create execution directory: $mkdir_message"
done

[[ -n "$execution_dir" ]] ||
    fail "No available execution ID for timestamp: $timestamp"
log_dir="${execution_dir}/logs"

mkdir -- \
    "${execution_dir}/root" \
    "${execution_dir}/conditions" \
    "${execution_dir}/inputs" \
    "$log_dir"

started_at="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
process_state="preparing"
process_exit_status="not_started"
simulation_pid=""

# ------------------------------------------------------------
# 正常終了・失敗のどちらでも終了情報を記録する
# ------------------------------------------------------------

finish() {
    # manifest の検証前に、実行処理が返した終了コード
    local script_exit_status=$?
    local final_exit_status="$script_exit_status"
    local manifest_exit_status=0

    trap - EXIT

    # 終了記録の途中で再度割り込まれないようにする
    trap '' INT TERM
    set +e

    if ! {
        printf 'END_TIME=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        printf 'PROCESS_STATE=%s\n' "$process_state"
        printf 'PROCESS_EXIT_STATUS=%s\n' "$process_exit_status"
        printf 'SCRIPT_EXIT_STATUS=%s\n' "$script_exit_status"
    } >> "${log_dir}/output.txt"; then
        echo "Failed to write execution termination log." >&2

        if [[ "$final_exit_status" -eq 0 ]]; then
            final_exit_status=1
        fi
    fi

    # 保存された Run の条件と出力先を集約する
    if python3 "$manifest_writer" \
        "$execution_dir" \
        --phase finished \
        > "${log_dir}/manifest_finish.txt" 2>&1; then
        manifest_exit_status=0
    else
        manifest_exit_status=$?

        cat "${log_dir}/manifest_finish.txt" >&2

        # 既存の失敗・割り込みコードは維持する。
        # 実行自体が成功でも、未完了や記録失敗なら非ゼロにする。
        if [[ "$final_exit_status" -eq 0 ]]; then
            final_exit_status="$manifest_exit_status"
        fi
    fi

    if ! {
        printf 'MANIFEST_EXIT_STATUS=%s\n' "$manifest_exit_status"
        printf 'FINAL_EXIT_STATUS=%s\n' "$final_exit_status"
    } >> "${log_dir}/output.txt"; then
        echo "Failed to write final exit status." >&2

        if [[ "$final_exit_status" -eq 0 ]]; then
            final_exit_status=1
        fi
    fi

    printf 'Results: %s\n' "$execution_dir"
    printf 'Manifest: %s/manifest.json\n' "$execution_dir"
    printf 'Final exit status: %s\n' "$final_exit_status"

    exit "$final_exit_status"
}

handle_signal() {
    local signal_exit_status="$1"

    # 終了処理中の再入を防ぐ
    trap '' INT TERM

    if [[ -n "$simulation_pid" ]]; then
        kill -TERM "$simulation_pid" 2>/dev/null || true

        if wait "$simulation_pid" 2>/dev/null; then
            process_exit_status=0
        else
            process_exit_status=$?
        fi

        simulation_pid=""
    fi

    process_state="interrupted"
    exit "$signal_exit_status"
}

trap finish EXIT
trap 'handle_signal 130' INT
trap 'handle_signal 143' TERM

{
    printf 'EXECUTION_ID=%s\n' "$execution_id"
    printf 'EXECUTION_DIR=%s\n' "$execution_dir"
    printf 'ROOT_NAMING_MODE=%s\n' "$naming"
    printf 'EXECUTABLE=%s\n' "$exe"
    printf 'ORIGINAL_MACRO=%s\n' "$macro"
    printf 'SAVED_MACRO=inputs/%s\n' "$macro_name"
    printf 'START_TIME=%s\n' "$started_at"
} > "${log_dir}/output.txt"

# ------------------------------------------------------------
# 入力とコード情報を保存する
# ------------------------------------------------------------

# 指定されたファイルだけを、相対的な配置を維持して保存する
: > "${log_dir}/inputs.txt"

for input_file in "${input_sources[@]}"; do
    relative_input="${input_file#"${input_dir}/"}"
    saved_input="${execution_dir}/inputs/${relative_input}"

    mkdir -p -- "$(dirname -- "$saved_input")"
    cp -pL -- "$input_file" "$saved_input"

    printf '%s\t%s\n' \
        "$input_file" \
        "inputs/${relative_input}" \
        >> "${log_dir}/inputs.txt"
done

{
    printf 'GIT_COMMIT='
    git -C "$project_dir" rev-parse HEAD 2>/dev/null ||
        echo "unknown"

    printf 'GIT_BRANCH='
    git -C "$project_dir" branch --show-current 2>/dev/null ||
        echo "unknown"

    printf '\n[git status --short]\n'
    git -C "$project_dir" status --short 2>/dev/null || true
} > "${log_dir}/version.txt"

git -C "$project_dir" diff \
    > "${log_dir}/git.diff" 2>/dev/null || true

# ------------------------------------------------------------
# Geant4 に出力設定を渡す
# ------------------------------------------------------------

unset G4_OUTPUT_ROOT G4_RUN_LABEL

export G4_EXECUTION_ID="$execution_id"
export G4_OUTPUT_DIR="$execution_dir"
export G4_OUTPUT_NAMING="$naming"
export G4_OUTPUT_PREPARED=1
export G4_LOG_DIR="$log_dir"

printf 'Execution ID: %s\n' "$execution_id"
printf 'ROOT naming mode: %s\n' "$naming"
printf 'Log: %s/stdout.txt\n' "$log_dir"


# シミュレーション開始前に、実行全体の記録を作成する
if python3 "$manifest_writer" \
    "$execution_dir" \
    --phase running \
    > "${log_dir}/manifest_start.txt" 2>&1; then
    :
else
    manifest_start_status=$?
    cat "${log_dir}/manifest_start.txt" >&2
    exit "$manifest_start_status"
fi

# ------------------------------------------------------------
# 保存した入力を使って実行する
# ------------------------------------------------------------

process_state="running"

(
    cd -- "${execution_dir}/inputs"
    exec "$exe" "$macro_name"
) > "${log_dir}/stdout.txt" 2>&1 < /dev/null &

simulation_pid=$!

# set -e が有効でも、失敗時の終了コードを取得する
if wait "$simulation_pid"; then
    process_exit_status=0
else
    process_exit_status=$?
fi

simulation_pid=""
process_state="exited"

exit "$process_exit_status"