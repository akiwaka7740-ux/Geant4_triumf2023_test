#!/usr/bin/env python3

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path


def read_execution_log(path):
    """shell が保存した KEY=VALUE 形式のログを読む。"""
    metadata = {}

    with path.open(encoding="utf-8") as stream:
        for line in stream:
            key, separator, value = line.rstrip("\n").partition("=")

            if separator:
                metadata[key] = value

    return metadata


def read_exit_status(value):
    if value is None or value == "not_started":
        return None

    return int(value)


def reject_nonfinite(value):
    raise ValueError(f"Invalid JSON number: {value}")


def collect_runs(directory, execution_id, naming_mode):
    runs = []
    issues = []
    seen_run_ids = set()
    seen_root_files = set()

    for path in sorted((directory / "conditions").glob("run*.json")):
        conditions_file = path.relative_to(directory).as_posix()

        try:
            with path.open(encoding="utf-8") as stream:
                data = json.load(
                    stream,
                    parse_constant=reject_nonfinite,
                )

            if not isinstance(data, dict):
                raise ValueError("Conditions must be a JSON object.")

            if data.get("schema_version") != 1:
                raise ValueError("Unsupported conditions schema.")

            if data.get("execution_id") != execution_id:
                raise ValueError("Execution ID does not match.")

            if data.get("root_naming_mode") != naming_mode:
                raise ValueError("ROOT naming mode does not match.")

            run_id = data["run_id"]
            requested = data["requested_events"]
            processed = data["processed_events"]
            status = data["status"]

            for name, value in (
                ("run_id", run_id),
                ("requested_events", requested),
                ("processed_events", processed),
            ):
                if type(value) is not int or value < 0:
                    raise ValueError(
                        f"{name} must be a non-negative integer."
                    )

            if path.name != f"run{run_id:03d}.json":
                raise ValueError("Filename does not match Run ID.")

            if run_id in seen_run_ids:
                raise ValueError("Duplicate Run ID.")

            if status not in (
                "running",
                "completed",
                "aborted",
                "failed",
            ):
                raise ValueError("Unknown Run status.")

            if status == "completed" and processed != requested:
                raise ValueError(
                    "Completed Run has inconsistent event counts."
                )

            root_file = data["root_file"]

            if not isinstance(root_file, str) or not root_file:
                raise ValueError("ROOT path must be a non-empty string.")

            root_relative = Path(root_file)

            if (
                root_relative.is_absolute()
                or ".." in root_relative.parts
                or not root_relative.parts
                or root_relative.parts[0] != "root"
                or root_relative.suffix != ".root"
            ):
                raise ValueError("Invalid relative ROOT path.")

            root_path = (directory / root_relative).resolve()

            # 実行ディレクトリの外を参照していないか確認
            root_path.relative_to(directory)

            if root_path in seen_root_files:
                raise ValueError("Multiple Runs refer to the same ROOT file.")

            root_exists = root_path.is_file()

            if status == "completed" and not root_exists:
                issues.append(
                    f"{conditions_file}: completed ROOT file is missing."
                )

            seen_run_ids.add(run_id)
            seen_root_files.add(root_path)

            runs.append({
                "run_id": run_id,
                "conditions_file": conditions_file,
                "root_file": root_relative.as_posix(),
                "root_file_exists": root_exists,
                "status": status,
                "requested_events": requested,
                "processed_events": processed,
            })

        except (OSError, ValueError, TypeError, KeyError) as error:
            issues.append(f"{conditions_file}: {error}")

    runs.sort(key=lambda record: record["run_id"])

    return runs, issues


def determine_status(
    phase,
    metadata,
    runs,
    issues,
    process_exit_status,
    script_exit_status,
):
    if phase == "running":
        return "failed" if issues else "running"

    if metadata.get("PROCESS_STATE") == "interrupted":
        return "interrupted"

    if (
        issues
        or process_exit_status != 0
        or script_exit_status != 0
    ):
        return "failed"

    if not runs:
        return "no_runs"

    statuses = {record["status"] for record in runs}

    if "failed" in statuses:
        return "failed"

    if "running" in statuses:
        return "incomplete"

    if "aborted" in statuses:
        return "aborted"

    return "completed"


def main():
    parser = argparse.ArgumentParser(
        description="Create an execution manifest from saved Run records."
    )

    parser.add_argument("execution_directory", type=Path)

    parser.add_argument(
        "--phase",
        required=True,
        choices=("running", "finished"),
    )

    args = parser.parse_args()

    try:
        directory = args.execution_directory.resolve()

        metadata = read_execution_log(
            directory / "logs" / "output.txt"
        )

        execution_id = metadata["EXECUTION_ID"]
        naming_mode = metadata["ROOT_NAMING_MODE"]

        process_exit_status = read_exit_status(
            metadata.get("PROCESS_EXIT_STATUS")
        )

        script_exit_status = read_exit_status(
            metadata.get("SCRIPT_EXIT_STATUS")
        )

        runs, issues = collect_runs(
            directory,
            execution_id,
            naming_mode,
        )

        status = determine_status(
            args.phase,
            metadata,
            runs,
            issues,
            process_exit_status,
            script_exit_status,
        )

        manifest = {
            "schema_version": 1,
            "execution_id": execution_id,
            "root_naming_mode": naming_mode,
            "status": status,
            "started_at": metadata.get("START_TIME"),
            "finished_at": metadata.get("END_TIME"),
            "updated_at": datetime.now(timezone.utc).isoformat(),
            "executable": metadata.get("EXECUTABLE"),
            "input_macro": metadata.get("SAVED_MACRO"),
            "stdout_file": "logs/stdout.txt",
            "execution_log": "logs/output.txt",
            "process_exit_status": process_exit_status,
            "script_exit_status": script_exit_status,
            "runs": runs,
            "issues": issues,
        }

        temporary_path = directory / "manifest.json.tmp"
        final_path = directory / "manifest.json"

        with temporary_path.open("w", encoding="utf-8") as stream:
            json.dump(
                manifest,
                stream,
                ensure_ascii=False,
                allow_nan=False,
                indent=2,
            )
            stream.write("\n")

        temporary_path.replace(final_path)

        print(f"Manifest: {final_path}")
        print(f"Execution status: {status}")

        for issue in issues:
            print(f"Manifest issue: {issue}", file=sys.stderr)

        if status in ("running", "completed"):
            return 0

        return 1

    except (OSError, ValueError, TypeError, KeyError) as error:
        print(
            f"Cannot create manifest: {error}",
            file=sys.stderr,
        )
        return 2


if __name__ == "__main__":
    sys.exit(main())