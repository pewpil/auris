"""Summarize operational Auris sensor and camera JSON Lines logs.

Purpose: Turn low-rate Pi bring-up snapshots into a concise, reproducible
diagnostic report without modifying device state or claiming study evidence.
Inputs: A telemetry JSONL path and an optional camera-luma JSONL path.
Outputs: A Markdown report to stdout or a requested file; distances use metres,
angles use degrees, image luma uses 0–255, and processing times use milliseconds.
"""

from __future__ import annotations

import argparse
import json
import math
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from statistics import median
from typing import Any, Iterable, Sequence


def read_jsonl(path: Path) -> tuple[list[dict[str, Any]], int]:
    """Read JSON object rows and count malformed or non-object lines.

    Args:
        path: JSON Lines file containing operational snapshots.

    Returns:
        Valid decoded objects and the number of rejected non-empty lines in
        the full input file, before any report-window filter is applied.

    Raises:
        OSError: If the requested file cannot be read.
    """
    records: list[dict[str, Any]] = []
    rejected = 0
    with path.open("r", encoding="utf-8") as source:
        for line in source:
            if not line.strip():
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                rejected += 1
                continue
            if isinstance(record, dict):
                records.append(record)
            else:
                rejected += 1
    return records, rejected


def parse_timestamp(value: Any) -> datetime | None:
    """Parse an ISO 8601 timestamp and normalize it to UTC.

    Args:
        value: Timestamp text from a recorder row.

    Returns:
        A timezone-aware UTC timestamp, or ``None`` when parsing fails.
    """
    if not isinstance(value, str):
        return None
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None
    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=timezone.utc)
    return parsed.astimezone(timezone.utc)


def filter_records_since(
    records: list[dict[str, Any]], cutoff: datetime | None
) -> list[dict[str, Any]]:
    """Keep timestamped rows at or after an optional UTC cutoff.

    Args:
        records: Decoded JSON objects from a bring-up log.
        cutoff: Inclusive timezone-aware cutoff, or ``None`` to keep all rows.

    Returns:
        Rows whose parseable UTC timestamp is at or after the cutoff.
    """
    if cutoff is None:
        return records
    return [
        record
        for record in records
        if (timestamp := parse_timestamp(record.get("timestamp_utc"))) is not None
        and timestamp >= cutoff
    ]


def finite_numbers(values: Iterable[Any]) -> list[float]:
    """Keep finite numeric values while excluding booleans and nulls.

    Args:
        values: Arbitrary JSON-decoded values.

    Returns:
        Finite values as floats, preserving their input order.
    """
    result: list[float] = []
    for value in values:
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            continue
        numeric = float(value)
        if math.isfinite(numeric):
            result.append(numeric)
    return result


def percentile(values: Sequence[float], fraction: float) -> float | None:
    """Compute a linearly interpolated percentile from finite numeric samples.

    Args:
        values: Samples in any order; values have the caller's units.
        fraction: Requested percentile from 0.0 through 1.0.

    Returns:
        The percentile in the input units, or ``None`` for no samples.

    Raises:
        ValueError: If the percentile fraction is outside 0.0 through 1.0.
    """
    if not 0.0 <= fraction <= 1.0:
        raise ValueError("percentile fraction must be between 0 and 1")
    ordered = sorted(finite_numbers(values))
    if not ordered:
        return None
    position = (len(ordered) - 1) * fraction
    lower = math.floor(position)
    upper = math.ceil(position)
    weight = position - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def row_value(record: dict[str, Any], section: str, key: str) -> Any:
    """Read a nested JSON value while treating malformed sections as absent.

    Args:
        record: One decoded state or camera-luma JSON object.
        section: Top-level object key, such as ``camera`` or ``pointer``.
        key: Field to read from the nested object.

    Returns:
        The nested value, or ``None`` if either level is unavailable.
    """
    nested = record.get(section)
    return nested.get(key) if isinstance(nested, dict) else None


def pointer_yaw_deg(record: dict[str, Any]) -> float | None:
    """Extract horizontal pointer aim azimuth from its row-major rotation.

    Args:
        record: A telemetry row whose ``pointer.matrix_head`` is a flattened
            row-major 3×3 rotation; the first column is the pointer aim axis in
            the published head frame.

    Returns:
        Aim azimuth in degrees as ``atan2(m10, m00)``, or ``None`` if invalid.
    """
    matrix = row_value(record, "pointer", "matrix_head")
    if not isinstance(matrix, list) or len(matrix) != 9:
        return None
    values = finite_numbers(matrix)
    if len(values) != 9 or math.hypot(values[0], values[3]) < 1e-9:
        return None
    return math.degrees(math.atan2(values[3], values[0]))


def fmt_number(value: float | None, digits: int = 2) -> str:
    """Format a finite measurement or return an em dash when unavailable.

    Args:
        value: Measurement in the units stated by its report label.
        digits: Decimal places to show.

    Returns:
        A fixed-point value or an em dash.
    """
    return f"{value:.{digits}f}" if value is not None and math.isfinite(value) else "—"


def summarize_state(records: list[dict[str, Any]], rejected: int) -> list[str]:
    """Build diagnostic report lines for sensor, tracking, and timing history.

    Report the latest and peak camera detection counters separately and count
    counter drops, so a service restart cannot hide earlier marker detections.

    Args:
        records: Decoded rows from the state recorder.
        rejected: Count of non-empty rows that could not be decoded as objects.

    Returns:
        Markdown lines describing operational availability and observed ranges.
    """
    lines = ["## State snapshots", f"- Valid rows in report window: {len(records)}; malformed rows in full source log: {rejected}."]
    errors = sum(1 for row in records if row.get("error"))
    timestamps = [parse_timestamp(row.get("timestamp_utc")) for row in records]
    timed = [(timestamp, row) for timestamp, row in zip(timestamps, records) if timestamp]
    lines.append(f"- Endpoint poll errors: {errors}.")
    if timed:
        span_s = (timed[-1][0] - timed[0][0]).total_seconds()
        lines.append(f"- Log window: {timed[0][0].isoformat()} to {timed[-1][0].isoformat()} UTC ({span_s / 60:.1f} min).")
        intervals = [(b[0] - a[0]).total_seconds() for a, b in zip(timed, timed[1:])]
        valid_intervals = finite_numbers(intervals)
        lines.append(f"- Snapshot cadence: median {fmt_number(median(valid_intervals) if valid_intervals else None)} s; longest gap {fmt_number(max(valid_intervals) if valid_intervals else None)} s.")

    successful = [row for row in records if not row.get("error")]
    pointer_rows = [row for row in successful if row_value(row, "pointer", "matrix_head")]
    head_fresh = sum(row_value(row, "head", "fresh") is True for row in successful)
    camera_fresh = sum(row_value(row, "camera", "position_fresh") is True for row in successful)
    lines.append(f"- Pointer orientation rows: {len(pointer_rows)}/{len(successful)}; head IMU fresh: {head_fresh}/{len(successful)}; camera pose fresh: {camera_fresh}/{len(successful)}.")

    pointer_sources = Counter(
        source for row in successful
        if isinstance((source := row_value(row, "pointer", "source")), str)
    )
    source_summary = ", ".join(
        f"{source}: {count}" for source, count in sorted(pointer_sources.items())
    ) or "not recorded"
    lines.append(f"- Pointer stream sources: {source_summary}.")
    pointer_schemas = Counter(
        schema for row in successful
        if isinstance((schema := row_value(row, "pointer", "schema")), str)
    )
    schema_summary = ", ".join(
        f"{schema}: {count}" for schema, count in sorted(pointer_schemas.items())
    ) or "not recorded"
    lines.append(f"- Decoded pointer row schemas: {schema_summary}.")
    head_sources = Counter(
        source for row in successful
        if isinstance((source := row_value(row, "head", "source")), str)
    )
    head_summary = ", ".join(
        f"{source}: {count}" for source, count in sorted(head_sources.items())
    ) or "not recorded"
    lines.append(f"- Configured head input: {head_summary}.")
    imu_known = [row for row in successful
                 if isinstance(row_value(row, "pointer", "imu_fresh"), bool)]
    tof_known = [row for row in successful
                 if isinstance(row_value(row, "pointer", "tof_fresh"), bool)]
    imu_fresh_count = sum(row_value(row, "pointer", "imu_fresh") is True
                          for row in imu_known)
    tof_fresh_count = sum(row_value(row, "pointer", "tof_fresh") is True
                          for row in tof_known)
    pointer_ages = finite_numbers(
        row_value(row, "pointer", "sample_age_s") for row in successful
    )
    lines.append(
        f"- Pointer freshness: IMU {imu_fresh_count}/{len(imu_known)} known rows; "
        f"ToF {tof_fresh_count}/{len(tof_known)} known rows; sample age "
        f"p50/p95 {fmt_number(percentile(pointer_ages, 0.50) * 1000 if pointer_ages else None, 1)}/"
        f"{fmt_number(percentile(pointer_ages, 0.95) * 1000 if pointer_ages else None, 1)} ms."
    )
    calibration_known = [row for row in successful
                         if isinstance(row_value(row, "pointer", "calibrated"), bool)]
    calibrated_count = sum(row_value(row, "pointer", "calibrated") is True
                           for row in calibration_known)
    firmware_flags = Counter(
        flags for row in successful
        if isinstance((flags := row_value(row, "pointer", "flags")), int)
        and not isinstance(flags, bool)
    )
    flag_summary = ", ".join(
        f"0x{flags:02x}: {count}" for flags, count in sorted(firmware_flags.items())
    ) or "not recorded"
    lines.append(
        f"- Pointer IMU calibration: {calibrated_count}/{len(calibration_known)} "
        f"known rows calibrated; firmware flags {flag_summary}."
    )
    accel_magnitudes: list[float] = []
    gyro_rows = 0
    for row in successful:
        accel = row_value(row, "pointer", "accel_g")
        if isinstance(accel, list) and len(accel) == 3:
            axes = finite_numbers(accel)
            if len(axes) == 3:
                accel_magnitudes.append(math.sqrt(sum(axis * axis for axis in axes)))
        gyro = row_value(row, "pointer", "gyro_dps")
        if isinstance(gyro, list) and len(gyro) == 3 and len(finite_numbers(gyro)) == 3:
            gyro_rows += 1
    range_ages_ms = finite_numbers(
        row_value(row, "pointer", "range_age_ms") for row in successful
    )
    range_statuses = Counter(
        status for row in successful
        if isinstance((status := row_value(row, "pointer", "range_status")), int)
        and not isinstance(status, bool)
    )
    status_summary = ", ".join(
        f"{status}: {count}" for status, count in sorted(range_statuses.items())
    ) or "not recorded"
    lines.append(
        f"- Raw pointer sensors: accelerometer norm p50/p95 "
        f"{fmt_number(percentile(accel_magnitudes, 0.50))}/"
        f"{fmt_number(percentile(accel_magnitudes, 0.95))} g; gyroscope vector "
        f"present in {gyro_rows}/{len(successful)} rows; ToF status {status_summary}; "
        f"measurement age p50/p95 {fmt_number(percentile(range_ages_ms, 0.50), 1)}/"
        f"{fmt_number(percentile(range_ages_ms, 0.95), 1)} ms."
    )

    ranges = finite_numbers(row_value(row, "pointer", "range_m") for row in successful)
    lines.append(f"- ToF range: {fmt_number(min(ranges) if ranges else None)}–{fmt_number(max(ranges) if ranges else None)} m; median {fmt_number(median(ranges) if ranges else None)} m.")
    triggered = sum(row_value(row, "pointer", "button") in (1, True, "1", "true") for row in successful)
    lines.append(f"- Trigger asserted in {triggered}/{len(successful)} snapshots.")

    camera_rows = [row for row in successful if isinstance(row.get("camera"), dict)]
    camera_timed: list[tuple[datetime, float]] = []
    for timestamp, row in timed:
        frame = finite_numbers([row_value(row, "camera", "frame_count")])
        if frame:
            camera_timed.append((timestamp, frame[0]))
    frame_values = [frame for _, frame in camera_timed]
    counter_delta = 0.0
    counter_span_s = 0.0
    for (previous_time, previous_frame), (current_time, current_frame) in zip(
        camera_timed, camera_timed[1:]
    ):
        elapsed_s = (current_time - previous_time).total_seconds()
        frame_delta = current_frame - previous_frame
        if elapsed_s > 0 and frame_delta >= 0:
            counter_delta += frame_delta
            counter_span_s += elapsed_s
    fps = counter_delta / counter_span_s if counter_span_s > 0 else None
    accepted = finite_numbers(row_value(row, "camera", "detection_count") for row in camera_rows)
    failures = finite_numbers(row_value(row, "camera", "read_failures") for row in camera_rows)
    marker_ids: set[int] = set()
    latest_ids: list[int] = []
    frame_counter_drops = 0
    detection_counter_drops = 0
    previous_frame_count: float | None = None
    previous_detection_count: float | None = None
    for row in camera_rows:
        frame_count = finite_numbers([row_value(row, "camera", "frame_count")])
        detection_count = finite_numbers([row_value(row, "camera", "detection_count")])
        if frame_count:
            if previous_frame_count is not None and frame_count[0] < previous_frame_count:
                frame_counter_drops += 1
            previous_frame_count = frame_count[0]
        if detection_count:
            if (previous_detection_count is not None
                    and detection_count[0] < previous_detection_count):
                detection_counter_drops += 1
            previous_detection_count = detection_count[0]
        row_ids = row_value(row, "camera", "marker_ids")
        if isinstance(row_ids, list):
            latest_ids = [
                marker_id
                for marker_id in row_ids
                if isinstance(marker_id, int) and not isinstance(marker_id, bool)
            ]
            marker_ids.update(latest_ids)
    accepted_ids = sorted(marker_ids)
    read_ms = finite_numbers(row_value(row, "camera", "read_ms") for row in camera_rows)
    detect_ms = finite_numbers(row_value(row, "camera", "detection_ms") for row in camera_rows)
    lines.append(
        f"- Camera: estimated {fmt_number(fps, 2)} fps from {len(frame_values)} counter samples; "
        f"detection counter last/max {fmt_number(accepted[-1], 0) if accepted else '—'}/"
        f"{fmt_number(max(accepted), 0) if accepted else '—'}; frame/detection counter drops "
        f"{frame_counter_drops}/{detection_counter_drops}; IDs seen in window "
        f"{accepted_ids or 'none'}; latest IDs {latest_ids or 'none'}."
    )
    camera_config = next(
        (row.get("camera") for row in reversed(successful)
         if isinstance(row.get("camera"), dict)
         and row_value(row, "camera", "dictionary_name")),
        None,
    )
    if isinstance(camera_config, dict):
        configured_ids = camera_config.get("target_marker_ids")
        configured_ids = [value for value in configured_ids
                          if isinstance(value, int) and not isinstance(value, bool)] if isinstance(configured_ids, list) else []
        lines.append(
            f"- Detector configuration: {camera_config.get('dictionary_name')}; "
            f"target IDs {configured_ids or 'not recorded'}; printed tag "
            f"{fmt_number(camera_config.get('marker_size_m') * 1000 if isinstance(camera_config.get('marker_size_m'), (int, float)) and not isinstance(camera_config.get('marker_size_m'), bool) else None, 0)} mm; cube "
            f"{fmt_number(camera_config.get('cube_size_m') * 1000 if isinstance(camera_config.get('cube_size_m'), (int, float)) and not isinstance(camera_config.get('cube_size_m'), bool) else None, 0)} mm."
        )
    else:
        lines.append("- Detector configuration: not recorded.")
    lines.append(f"- Camera frame-read failures: {fmt_number(max(failures) if failures else None, 0)}; read time p50/p95 {fmt_number(percentile(read_ms, 0.50))}/{fmt_number(percentile(read_ms, 0.95))} ms; detect time p50/p95 {fmt_number(percentile(detect_ms, 0.50))}/{fmt_number(percentile(detect_ms, 0.95))} ms.")

    yaw_samples = [(timestamp, pointer_yaw_deg(row)) for timestamp, row in timed]
    yaw_samples = [(timestamp, yaw) for timestamp, yaw in yaw_samples if yaw is not None]
    if len(yaw_samples) >= 2:
        unwrapped_delta = 0.0
        directional_steps: list[float] = []
        for (_, previous), (_, current) in zip(yaw_samples, yaw_samples[1:]):
            step = (current - previous + 180.0) % 360.0 - 180.0
            unwrapped_delta += step
            directional_steps.append(step)
        elapsed_min = (yaw_samples[-1][0] - yaw_samples[0][0]).total_seconds() / 60.0
        rate = unwrapped_delta / elapsed_min if elapsed_min > 0 else None
        positive_steps = sum(step > 0.0 for step in directional_steps)
        lines.append(f"- Pointer aim azimuth changed {unwrapped_delta:+.1f}° across {elapsed_min:.1f} min ({fmt_number(rate, 2)}°/min); positive steps {positive_steps}/{len(directional_steps)}.")
        lines.append("- Yaw-change interpretation is conditional on the pointer being physically still; this log contains no independent motion reference.")
    else:
        lines.append("- Pointer aim azimuth trend: insufficient valid rotation samples.")

    lines.append("- These 30 s operational snapshots are not event-level study instrumentation and do not establish tracking accuracy, sensor drift, or end-to-end latency quality.")
    return lines


def summarize_luma(records: list[dict[str, Any]], rejected: int) -> list[str]:
    """Build report lines for the low-rate camera brightness history.

    Args:
        records: Decoded rows from the camera-luma recorder.
        rejected: Count of non-empty rows that could not be decoded as objects.

    Returns:
        Markdown lines with luma sample count and observed brightness range.
    """
    samples = [row for row in records if not row.get("error")]
    means = finite_numbers(row.get("mean_luma_0_255") for row in samples)
    medians = finite_numbers(row.get("p50_luma_0_255") for row in samples)
    lines = ["## Camera brightness snapshots", f"- Valid rows in report window: {len(records)}; malformed rows in full source log: {rejected}; image decode errors in window: {sum(bool(row.get('error')) for row in records)}."]
    if means:
        lines.append(f"- Mean frame luma range: {min(means):.2f}–{max(means):.2f}/255; median of frame medians: {fmt_number(median(medians) if medians else None)}/255.")
        lines.append("- Luma is scene telemetry only; it cannot distinguish a covered lens, aim direction, exposure behaviour, or marker visibility by itself.")
    else:
        lines.append("- No valid camera brightness samples were recorded.")
    return lines


def build_report(
    state_path: Path,
    camera_path: Path | None,
    state_records: list[dict[str, Any]],
    state_rejected: int,
    camera_records: list[dict[str, Any]] | None,
    camera_rejected: int,
    since_utc: str | None,
) -> str:
    """Assemble a Markdown bring-up summary from the selected log files.

    Args:
        state_path: Path used for the state snapshot log.
        camera_path: Optional camera brightness log path.
        state_records: Decoded state snapshot objects.
        state_rejected: Rejected state log line count.
        camera_records: Optional decoded camera brightness objects.
        camera_rejected: Rejected camera log line count.
        since_utc: Optional inclusive UTC timestamp used to select the report window.

    Returns:
        A Markdown report with one unwrapped line per paragraph and list item.
    """
    generated = datetime.now(timezone.utc).isoformat(timespec="seconds")
    lines = ["# Auris bring-up telemetry report", "", f"Generated: {generated} UTC.", "", f"State log: `{state_path}`."]
    if camera_path:
        lines.append(f"Camera-luma log: `{camera_path}`.")
    if since_utc:
        lines.append(f"Rows are filtered from `{since_utc}` UTC onward.")
    lines.extend(["", *summarize_state(state_records, state_rejected)])
    if camera_records is not None:
        lines.extend(["", *summarize_luma(camera_records, camera_rejected)])
    lines.extend(["", "## Interpretation", "", "This report describes observed operational telemetry only. Apply the repository's evidence labels to claims made in the design; a logged value does not become measured study evidence merely because it was collected from hardware."])
    return "\n".join(lines) + "\n"


def parse_arguments(argv: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse telemetry inputs and the optional Markdown report destination.

    Args:
        argv: Optional argument sequence; ``None`` reads command-line arguments.

    Returns:
        Input paths and optional output path.

    Raises:
        SystemExit: If required arguments are missing or malformed.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state-log", type=Path, required=True)
    parser.add_argument("--camera-log", type=Path)
    parser.add_argument("--since-utc", help="include rows at or after this ISO 8601 UTC timestamp")
    parser.add_argument("--report", type=Path, help="write Markdown to this path instead of stdout")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    """Read bring-up logs and write a diagnostic Markdown report.

    Args:
        argv: Optional argument sequence; ``None`` reads process arguments.

    Returns:
        Zero after successfully writing the report.

    Raises:
        OSError: If an input file cannot be read or a report cannot be written.
    """
    arguments = parse_arguments(argv)
    cutoff = parse_timestamp(arguments.since_utc) if arguments.since_utc else None
    if arguments.since_utc and cutoff is None:
        raise SystemExit("--since-utc must be an ISO 8601 timestamp")
    state_records, state_rejected = read_jsonl(arguments.state_log)
    state_records = filter_records_since(state_records, cutoff)
    camera_records = None
    camera_rejected = 0
    if arguments.camera_log:
        camera_records, camera_rejected = read_jsonl(arguments.camera_log)
        camera_records = filter_records_since(camera_records, cutoff)
    report = build_report(
        arguments.state_log,
        arguments.camera_log,
        state_records,
        state_rejected,
        camera_records,
        camera_rejected,
        arguments.since_utc,
    )
    if arguments.report:
        arguments.report.parent.mkdir(parents=True, exist_ok=True)
        arguments.report.write_text(report, encoding="utf-8")
    else:
        print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
