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