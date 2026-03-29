#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import shlex
import subprocess
import sys


REPO_ROOT = pathlib.Path(__file__).resolve().parents[3]
DEFAULT_ROM_PATH = REPO_ROOT / "build" / "showcase" / "showcase.rom"
DEFAULT_SYMBOL_PATH = REPO_ROOT / "build" / "showcase" / "showcase.sym"
HEADLESS_CANDIDATES = (
    REPO_ROOT / "build" / "vanguard8_headless",
    REPO_ROOT / "build" / "src" / "vanguard8_headless",
)
VALUE_OPTIONS = {
    "--frames",
    "--vclk",
    "--recent",
    "--replay",
    "--hash-frame",
    "--expect-frame-hash",
    "--expect-audio-hash",
    "--press-key",
    "--gamepad1-button",
    "--gamepad2-button",
}
PAIR_VALUE_OPTIONS = {"--expect-frame-hash"}


def usage() -> str:
    return (
        "Usage: python3 showcase/tools/package/run_showcase_headless.py "
        "[--rom path] [--symbols path] [--trace path] [--trace-instructions N] "
        "[--headless-bin path] [headless options...]\n"
        "\n"
        "Stable defaults:\n"
        f"  ROM: {DEFAULT_ROM_PATH.relative_to(REPO_ROOT)}\n"
        f"  Symbols: {DEFAULT_SYMBOL_PATH.relative_to(REPO_ROOT)} (auto-used for trace when present)\n"
        "\n"
        "Wrapper behavior:\n"
        "  - forwards runtime options to vanguard8_headless\n"
        "  - when --trace is combined with runtime options, runs the fixed-frame runtime first\n"
        "    and then captures a separate symbol-aware trace\n"
    )


def locate_headless_binary(explicit_path: pathlib.Path | None) -> pathlib.Path:
    if explicit_path is not None:
        if explicit_path.is_file():
            return explicit_path
        raise FileNotFoundError(f"Headless binary not found: {explicit_path}")

    for candidate in HEADLESS_CANDIDATES:
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "Unable to locate vanguard8_headless under build/. Build the emulator first."
    )


def run_command(argv: list[str]) -> None:
    print("$ " + " ".join(shlex.quote(arg) for arg in argv))
    subprocess.run(argv, cwd=REPO_ROOT, check=True)


def main(argv: list[str]) -> int:
    if "--help" in argv:
        print(usage(), end="")
        return 0

    rom_path = DEFAULT_ROM_PATH
    symbol_path: pathlib.Path | None = None
    trace_path: pathlib.Path | None = None
    trace_instructions: str | None = None
    headless_override: pathlib.Path | None = None
    passthrough: list[str] = []

    index = 0
    while index < len(argv):
        arg = argv[index]

        if arg == "--headless-bin":
            headless_override = pathlib.Path(argv[index + 1]).resolve()
            index += 2
            continue
        if arg == "--rom":
            rom_path = pathlib.Path(argv[index + 1]).resolve()
            index += 2
            continue
        if arg == "--symbols":
            symbol_path = pathlib.Path(argv[index + 1]).resolve()
            index += 2
            continue
        if arg == "--trace":
            trace_path = pathlib.Path(argv[index + 1]).resolve()
            index += 2
            continue
        if arg == "--trace-instructions":
            trace_instructions = argv[index + 1]
            index += 2
            continue

        passthrough.append(arg)
        if arg in PAIR_VALUE_OPTIONS:
            passthrough.append(argv[index + 1])
            passthrough.append(argv[index + 2])
            index += 3
            continue
        if arg in VALUE_OPTIONS:
            passthrough.append(argv[index + 1])
            index += 2
            continue
        index += 1

    if not rom_path.is_file():
        raise FileNotFoundError(
            f"Showcase ROM not found: {rom_path}. Run build_showcase.py first."
        )

    headless_binary = locate_headless_binary(headless_override)
    runtime_args = [str(headless_binary), "--rom", str(rom_path), *passthrough]

    if trace_path is None:
        run_command(runtime_args)
        return 0

    trace_path.parent.mkdir(parents=True, exist_ok=True)

    if passthrough:
        run_command(runtime_args)

    effective_symbol_path = symbol_path
    if effective_symbol_path is None and DEFAULT_SYMBOL_PATH.is_file():
        effective_symbol_path = DEFAULT_SYMBOL_PATH

    trace_args = [str(headless_binary), "--rom", str(rom_path), "--trace", str(trace_path)]
    effective_trace_instructions = trace_instructions or "32"
    trace_args.extend(["--trace-instructions", effective_trace_instructions])
    if effective_symbol_path is not None:
        trace_args.extend(["--symbols", str(effective_symbol_path)])

    run_command(trace_args)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode) from error
    except Exception as error:  # pragma: no cover - wrapper error path
        print(f"run_showcase_headless.py error: {error}", file=sys.stderr)
        raise SystemExit(1) from error
