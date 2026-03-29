#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import re
import shlex
import subprocess
import sys
import tempfile


REPO_ROOT = pathlib.Path(__file__).resolve().parents[3]
SOURCE_PATH = REPO_ROOT / "showcase" / "src" / "showcase.asm"
OUTPUT_DIR = REPO_ROOT / "build" / "showcase"
ROM_PATH = OUTPUT_DIR / "showcase.rom"
SYM_PATH = OUTPUT_DIR / "showcase.sym"
SYMBOL_PATTERN = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):\s+EQU\s+0x([0-9A-Fa-f]+)\s*$")


def run_command(argv: list[str]) -> None:
    print("$ " + " ".join(shlex.quote(arg) for arg in argv))
    subprocess.run(argv, cwd=REPO_ROOT, check=True)


def convert_sjasm_symbols(raw_symbol_path: pathlib.Path, output_symbol_path: pathlib.Path) -> int:
    entries: list[tuple[int, str]] = []
    seen_addresses: set[int] = set()
    seen_labels: set[str] = set()

    for raw_line in raw_symbol_path.read_text(encoding="utf-8").splitlines():
        match = SYMBOL_PATTERN.match(raw_line.strip())
        if match is None:
            continue

        label, address_hex = match.groups()
        if label == "org":
            continue

        address = int(address_hex, 16) & 0xFFFF
        if label in seen_labels or address in seen_addresses:
            continue

        seen_labels.add(label)
        seen_addresses.add(address)
        entries.append((address, label))

    entries.sort(key=lambda item: (item[0], item[1]))
    if not entries:
        raise RuntimeError("Assembler symbol output did not contain any usable labels.")

    output_symbol_path.write_text(
        "".join(f"{address:04X} {label}\n" for address, label in entries),
        encoding="utf-8",
    )
    return len(entries)


def main() -> int:
    if not SOURCE_PATH.is_file():
        raise FileNotFoundError(f"Showcase source file not found: {SOURCE_PATH}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    with tempfile.NamedTemporaryFile(
        prefix="showcase-",
        suffix=".sjasm.sym",
        dir=OUTPUT_DIR,
        delete=False,
    ) as tmp_file:
        raw_symbol_path = pathlib.Path(tmp_file.name)

    try:
        run_command(
            [
                "sjasmplus",
                "--nologo",
                "--msg=war",
                f"--raw={ROM_PATH}",
                f"--sym={raw_symbol_path}",
                str(SOURCE_PATH),
            ]
        )
        symbol_count = convert_sjasm_symbols(raw_symbol_path, SYM_PATH)
    finally:
        raw_symbol_path.unlink(missing_ok=True)

    print(f"ROM image: {ROM_PATH.relative_to(REPO_ROOT)}")
    print(f"Symbol file: {SYM_PATH.relative_to(REPO_ROOT)}")
    print(f"Symbol count: {symbol_count}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode) from error
    except Exception as error:  # pragma: no cover - wrapper error path
        print(f"build_showcase.py error: {error}", file=sys.stderr)
        raise SystemExit(1) from error
