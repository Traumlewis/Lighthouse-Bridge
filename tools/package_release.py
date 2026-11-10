#!/usr/bin/env python3
"""Package an existing PlatformIO build; never accesses or flashes hardware."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile
from ota_package import package

ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.3.8"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pio-core-dir", type=Path,
                        default=Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio")))
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    build = ROOT / ".pio/build/esp32dev"
    core = args.pio_core_dir
    parts = [
        ("0x1000", build / "bootloader.bin"),
        ("0x8000", build / "partitions.bin"),
        ("0xe000", core / "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"),
        ("0x10000", build / "firmware.bin"),
    ]
    for _, file in parts:
        if not file.is_file():
            raise SystemExit(f"Missing {file}; run pio run first.")
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temp:
        merged = Path(temp) / "lighthouse-bridge-esp32-merged.bin"
        subprocess.run([
            sys.executable, str(core / "packages/tool-esptoolpy/esptool.py"),
            "--chip", "esp32", "merge_bin", "-o", str(merged),
            "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB",
            *[value for address, file in parts for value in (address, str(file))],
        ], check=True)
        payload = merged.read_bytes()
        for address, file in parts:
            offset = int(address, 16)
            assert payload[offset:offset + file.stat().st_size] == file.read_bytes(), file
        checksum = hashlib.sha256(payload).hexdigest()
        # A merged image contains 0xff padding over NVS (0x9000..0xe000).
        # Writing it at zero erases settings even when eraseAll is false!
        # Web updates must flash only these four sector-aligned regions.
        segments = {}
        for (address, file), name in zip(parts, [
            "bootloader", "partitions", "boot-app0", "app"]):
            offset = int(address, 16)
            segments[f"lighthouse-bridge-{name}.bin"] = payload[offset:offset + file.stat().st_size]
        segments[f"lighthouse-bridge-v{VERSION}.lhota"] = package(segments["lighthouse-bridge-app.bin"])
        checksums = f"{checksum}  {merged.name}\n" + "".join(
            f"{hashlib.sha256(data).hexdigest()}  {name}\n" for name, data in segments.items())
        archive = args.output / f"lighthouse-bridge-v{VERSION}.zip"
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
            bundle.write(merged, merged.name)
            bundle.writestr("SHA256SUMS", checksums)
            for name, data in segments.items():
                bundle.writestr(name, data)
            for file in sorted(ROOT.rglob("*")):
                if not file.is_file():
                    continue
                relative = file.relative_to(ROOT)
                if any(part in {".pio", "dist", "_site", "node_modules", ".venv", "__pycache__", ".git"} for part in relative.parts):
                    continue
                if file.resolve() == archive.resolve():
                    continue
                bundle.write(file, Path("source") / relative)
            bundle.write(ROOT / "README.md", "READ-ME-FIRST.md")
            bundle.write(ROOT / "VERSION.md", "VERSION.md")
            # Include the license/notice files shipped with compiled dependencies.
            dep_roots = [ROOT / ".pio/libdeps/esp32dev", core / "packages/framework-arduinoespressif32"]
            for dep_root in dep_roots:
                for file in sorted(dep_root.rglob("*")):
                    if file.is_file() and file.name.lower().startswith(("license", "notice", "copying")):
                        bundle.write(file, Path("third-party-licenses") / dep_root.name / file.relative_to(dep_root))
        # Also supply the merged binary directly for convenient flashing.
        (args.output / merged.name).write_bytes(payload)
        for name, data in segments.items():
            (args.output / name).write_bytes(data)
        (args.output / "SHA256SUMS").write_text(checksums)
        with zipfile.ZipFile(archive) as bundle:
            assert bundle.testzip() is None
        print(f"Release: {archive}")
        print(f"Merged image: {len(payload)} bytes; all four flash segments verified.")
        print(f"SHA256: {checksum}")


if __name__ == "__main__":
    main()
