#!/usr/bin/env python3
"""Compile bgfx .sc shaders and pack selected backends into SCBP v1 files."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import Sequence


MAGIC = b"SCBP"
VERSION = 1
HEADER = struct.Struct("<4sIII")  # magic, version, stage, uint32 backend flags
ENTRY = struct.Struct("<II")  # absolute payload offset, payload size
UINT32_MAX = (1 << 32) - 1
STAGES = {"vertex": 1, "fragment": 2, "compute": 3}
STAGE_NAMES = {identifier: name for name, identifier in STAGES.items()}
SHADER_MAGIC = {"vertex": b"VSH", "fragment": b"FSH", "compute": b"CSH"}
REPOSITORY = Path(__file__).resolve().parent.parent


class ScbpError(Exception):
    """Invalid input, compiler failure, or invalid SCBP data."""


@dataclass(frozen=True)
class Target:
    name: str
    flag: int
    platform: str
    profile: str
    compute_profile: str


# These bits are the SCBP ABI; do not use bgfx::RendererType enum ordinals.
TARGETS = (
    Target("dx", 1 << 0, "windows", "s_5_0", "s_5_0"),
    Target("metal", 1 << 1, "osx", "metal", "metal"),
    Target("opengl", 1 << 2, "linux", "150", "430"),
    Target("opengles", 1 << 3, "android", "300_es", "310_es"),
    Target("vulkan", 1 << 4, "linux", "spirv", "spirv"),
)
TARGET_BY_NAME = {target.name: target for target in TARGETS}
KNOWN_FLAGS = sum(target.flag for target in TARGETS)


@dataclass(frozen=True)
class Segment:
    target: Target
    offset: int
    binary: bytes


@dataclass(frozen=True)
class Package:
    stage: str
    flags: int
    segments: tuple[Segment, ...]


def targets_from_flags(flags: int) -> tuple[Target, ...]:
    if flags <= 0 or flags > UINT32_MAX:
        raise ScbpError("backend flags must be a nonzero uint32")
    if flags & ~KNOWN_FLAGS:
        raise ScbpError(f"unsupported backend bits: 0x{flags & ~KNOWN_FLAGS:08x}")
    return tuple(target for target in TARGETS if flags & target.flag)


def parse_flags(value: str) -> int:
    try:
        flags = int(value, 0)
        targets_from_flags(flags)
        return flags
    except (ValueError, ScbpError) as error:
        raise argparse.ArgumentTypeError(str(error)) from error


def parse_targets(value: str) -> int:
    flags = 0
    for name in value.split(","):
        target = TARGET_BY_NAME.get(name.strip().lower())
        if target is None:
            raise argparse.ArgumentTypeError(
                f"unknown backend {name!r}; choose {', '.join(TARGET_BY_NAME)}"
            )
        if flags & target.flag:
            raise argparse.ArgumentTypeError(f"duplicate backend: {target.name}")
        flags |= target.flag
    return flags


def validate_binary(binary: bytes, stage: str) -> int:
    # bgfx shaderc writes a stage tag, a version byte and two interface hashes.
    if len(binary) < 12 or binary[:3] != SHADER_MAGIC[stage]:
        raise ScbpError(f"expected a bgfx shaderc {stage} binary ({SHADER_MAGIC[stage].decode()})")
    return binary[3]


def make_package(stage: str, binaries: dict[str, bytes]) -> bytes:
    if stage not in STAGES:
        raise ScbpError(f"unknown shader stage: {stage}")
    unknown = binaries.keys() - TARGET_BY_NAME.keys()
    if unknown:
        raise ScbpError(f"unknown backend: {', '.join(sorted(unknown))}")
    flags = sum(TARGET_BY_NAME[name].flag for name in binaries)
    selected = targets_from_flags(flags)
    versions = {validate_binary(binaries[target.name], stage) for target in selected}
    if len(versions) != 1:
        raise ScbpError("all segments must use the same bgfx shader binary version")
    offset = HEADER.size + ENTRY.size * len(selected)
    table = bytearray()
    payloads = []
    for target in selected:
        binary = binaries[target.name]
        if len(binary) > UINT32_MAX or offset + len(binary) > UINT32_MAX:
            raise ScbpError("SCBP v1 files must be smaller than 4 GiB")
        table.extend(ENTRY.pack(offset, len(binary)))
        payloads.append(binary)
        offset += len(binary)
    return HEADER.pack(MAGIC, VERSION, STAGES[stage], flags) + table + b"".join(payloads)


def read_package(data: bytes) -> Package:
    if len(data) < HEADER.size or len(data) > UINT32_MAX:
        raise ScbpError("invalid SCBP file size")
    magic, version, stage_id, flags = HEADER.unpack_from(data)
    if magic != MAGIC:
        raise ScbpError("invalid SCBP magic")
    if version != VERSION:
        raise ScbpError(f"unsupported SCBP version: {version}")
    stage = STAGE_NAMES.get(stage_id)
    if stage is None:
        raise ScbpError(f"invalid shader stage: {stage_id}")
    selected = targets_from_flags(flags)
    expected_offset = HEADER.size + ENTRY.size * len(selected)
    if len(data) < expected_offset:
        raise ScbpError("truncated SCBP segment table")
    segments = []
    versions = set()
    for index, target in enumerate(selected):
        offset, size = ENTRY.unpack_from(data, HEADER.size + ENTRY.size * index)
        if offset != expected_offset or size < 12 or size > len(data) - offset:
            raise ScbpError(f"invalid {target.name} segment offset or size")
        binary = data[offset:offset + size]
        versions.add(validate_binary(binary, stage))
        segments.append(Segment(target, offset, binary))
        expected_offset += size
    if expected_offset != len(data):
        raise ScbpError("unexpected trailing bytes")
    if len(versions) != 1:
        raise ScbpError("mixed bgfx shader binary versions")
    return Package(stage, flags, tuple(segments))


def read_binary(path: Path) -> bytes:
    if path.stat().st_size > UINT32_MAX:
        raise ScbpError(f"file exceeds the SCBP v1 size limit: {path}")
    return path.read_bytes()


def write_package(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + ".", suffix=".tmp", delete=False) as output:
            temporary = Path(output.name)
            output.write(data)
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def installed_prefixes() -> list[Path]:
    roots = [REPOSITORY / "vcpkg_installed"]
    if os.environ.get("VCPKG_ROOT"):
        roots.append(Path(os.environ["VCPKG_ROOT"]) / "installed")
    prefixes = []
    for root in roots:
        if root.is_dir():
            # MSBuild may install at either <triplet>/ or <triplet>/<triplet>/.
            prefixes.extend(sorted(root.glob("*")))
            prefixes.extend(sorted(root.glob("*/*")))
    return prefixes


def find_shaderc(requested: str | None) -> Path:
    if requested:
        path = Path(requested).expanduser()
        if path.is_file():
            return path.resolve()
        found = shutil.which(requested)
        if found:
            return Path(found).resolve()
        raise ScbpError(f"shaderc not found: {requested}")
    executable = "shaderc.exe" if os.name == "nt" else "shaderc"
    for prefix in installed_prefixes():
        path = prefix / "tools" / "bgfx" / executable
        if path.is_file():
            return path.resolve()
    found = shutil.which(executable)
    if found:
        return Path(found).resolve()
    raise ScbpError("shaderc not found; use --shaderc PATH or install bgfx[tools]")


def parse_overrides(values: list[str], label: str) -> dict[str, str]:
    overrides = {}
    for value in values:
        name, separator, setting = value.partition("=")
        name = name.strip().lower()
        if not separator or name not in TARGET_BY_NAME or not setting:
            raise ScbpError(f"expected {label} BACKEND=VALUE, got {value!r}")
        if name in overrides:
            raise ScbpError(f"duplicate {label} for {name}")
        overrides[name] = setting
    return overrides


def infer_stage(source: Path, explicit: str | None) -> str:
    suffixes = {".vert.sc": "vertex", ".frag.sc": "fragment", ".comp.sc": "compute"}
    inferred = next((stage for suffix, stage in suffixes.items() if source.name.lower().endswith(suffix)), None)
    if explicit and inferred and explicit != inferred:
        raise ScbpError(f"--type {explicit} conflicts with source filename: {source}")
    if explicit or inferred:
        return explicit or inferred
    raise ScbpError(f"cannot infer stage from {source}; name it *.vert.sc / *.frag.sc / *.comp.sc or use --type")


def validate_profile(target: Target, profile: str) -> None:
    matches = {
        "dx": profile in ("s_4_0", "s_5_0"),
        "metal": profile.startswith("metal"),
        "opengl": profile.isascii() and profile.isdecimal(),
        "opengles": profile.endswith("_es") and profile[:-3].isascii() and profile[:-3].isdecimal(),
        "vulkan": profile.startswith("spirv"),
    }
    if not matches[target.name]:
        raise ScbpError(f"profile {profile!r} does not belong to the {target.name} backend")


def compile_sources(args: argparse.Namespace) -> None:
    selected = targets_from_flags(args.flags)
    shaderc = find_shaderc(args.shaderc)
    profiles = parse_overrides(args.profile, "--profile")
    platforms = parse_overrides(args.platform, "--platform")
    unused = (profiles.keys() | platforms.keys()) - {target.name for target in selected}
    if unused:
        raise ScbpError(f"overrides for unselected backends: {', '.join(sorted(unused))}")
    includes = [path.resolve() for path in args.include]
    if any(not path.is_dir() for path in includes):
        raise ScbpError("every -i / --include path must be an existing directory")
    bgfx_include = shaderc.parent.parent.parent / "include" / "bgfx"
    if not (bgfx_include / "bgfx_shader.sh").is_file():
        bgfx_include = next(
            (prefix / "include" / "bgfx" for prefix in installed_prefixes()
             if (prefix / "include" / "bgfx" / "bgfx_shader.sh").is_file()),
            bgfx_include,
        )
    if (bgfx_include / "bgfx_shader.sh").is_file() and bgfx_include not in includes:
        includes.append(bgfx_include)
    if args.output and len(args.sources) != 1:
        raise ScbpError("-o / --output requires exactly one source; use --output-dir for multiple sources")
    jobs = []
    outputs = set()
    source_paths = {source.resolve() for source in args.sources}
    for source in args.sources:
        if source.suffix.lower() != ".sc" or not source.is_file():
            raise ScbpError(f"expected an existing .sc source: {source}")
        source = source.resolve()
        stage = infer_stage(source, args.type)
        output = args.output or (args.output_dir / source.with_suffix(".scbp").name if args.output_dir else source.with_suffix(".scbp"))
        output = output.resolve()
        if output in outputs or output in source_paths:
            raise ScbpError(f"duplicate output or output would overwrite a source: {output}")
        outputs.add(output)
        varying = args.varyingdef.resolve() if args.varyingdef else source.parent / "varying.def.sc"
        if (args.varyingdef or stage != "compute") and not varying.is_file():
            raise ScbpError(f"varying definition not found: {varying}; use --varyingdef PATH")
        jobs.append((source, stage, output, varying))
    packages = []
    # Finish all requested compilations before replacing any output package.
    with tempfile.TemporaryDirectory(prefix="scbp-") as temporary:
        for source_index, (source, stage, output, varying) in enumerate(jobs):
            binaries = {}
            for target in selected:
                binary_path = Path(temporary) / f"{source_index}-{target.name}.bin"
                profile = profiles.get(target.name, target.compute_profile if stage == "compute" else target.profile)
                validate_profile(target, profile)
                platform = platforms.get(target.name, target.platform)
                command = [str(shaderc), "-f", str(source), "-o", str(binary_path),
                           "--type", stage, "--platform", platform, "-p", profile, "-O", str(args.optimize)]
                for include in [source.parent, *includes]:
                    command.extend(["-i", str(include)])
                if varying.is_file():
                    command.extend(["--varyingdef", str(varying)])
                if args.define:
                    command.extend(["--define", ";".join(args.define)])
                if args.debug:
                    command.append("--debug")
                if args.werror:
                    command.append("--Werror")
                print(f"Compiling {source.name}: {target.name} ({platform}, {profile})", flush=True)
                result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
                if result.returncode or not binary_path.is_file():
                    diagnostic = (result.stdout + result.stderr).strip()
                    raise ScbpError(f"shaderc failed for {source.name}/{target.name} (exit {result.returncode})\n{diagnostic}")
                if result.stdout.strip():
                    print(result.stdout.strip())
                if result.stderr.strip():
                    print(result.stderr.strip(), file=sys.stderr)
                binaries[target.name] = read_binary(binary_path)
            packages.append((output, make_package(stage, binaries)))
    for output, data in packages:
        write_package(output, data)
        print(f"Wrote {output} ({len(data)} bytes, flags=0x{args.flags:08x})")


def pack_binaries(args: argparse.Namespace) -> None:
    inputs = parse_overrides(args.binary, "--binary")
    if args.output.resolve() in {Path(path).resolve() for path in inputs.values()}:
        raise ScbpError("output would overwrite an input binary")
    binaries = {name: read_binary(Path(path)) for name, path in inputs.items()}
    data = make_package(args.type, binaries)
    package = read_package(data)
    if args.flags is not None and package.flags != args.flags:
        raise ScbpError(f"binary backends produce flags 0x{package.flags:08x}, expected 0x{args.flags:08x}")
    write_package(args.output, data)
    print(f"Wrote {args.output} ({len(data)} bytes, flags=0x{package.flags:08x})")


def inspect_package(args: argparse.Namespace) -> None:
    data = read_binary(args.input)
    package = read_package(data)
    print(f"{args.input}: SCBP v{VERSION}, {package.stage}, flags=0x{package.flags:08x}, {len(data)} bytes")
    for segment in package.segments:
        print(f"  {segment.target.name:9} bit=0x{segment.target.flag:08x} offset={segment.offset} size={len(segment.binary)} bgfx-version={segment.binary[3]}")


def add_selection(parser: argparse.ArgumentParser, required: bool) -> None:
    selection = parser.add_mutually_exclusive_group(required=required)
    selection.add_argument("--targets", dest="flags", type=parse_targets, metavar="BACKENDS",
                           help="comma-separated dx,metal,opengl,opengles,vulkan")
    selection.add_argument("--flags", type=parse_flags, metavar="UINT32", help="backend mask, e.g. 0x3 for dx+metal")


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    build = commands.add_parser("build", help="compile .sc sources, then pack each stage")
    build.add_argument("sources", type=Path, nargs="+")
    add_selection(build, required=True)
    output = build.add_mutually_exclusive_group()
    output.add_argument("-o", "--output", type=Path, help="one output file for one source")
    output.add_argument("--output-dir", type=Path, help="destination for multiple stages")
    build.add_argument("--type", choices=STAGES, help="required when the stage is not in the filename")
    build.add_argument("--shaderc", help="bgfx shaderc executable; automatically finds local vcpkg installs")
    build.add_argument("--varyingdef", type=Path, help="defaults to varying.def.sc beside each source")
    build.add_argument("-i", "--include", type=Path, action="append", default=[])
    build.add_argument("--profile", action="append", default=[], metavar="BACKEND=PROFILE")
    build.add_argument("--platform", action="append", default=[], metavar="BACKEND=PLATFORM")
    build.add_argument("--define", action="append", default=[], metavar="NAME[=VALUE]")
    build.add_argument("-O", "--optimize", type=int, choices=range(4), default=3)
    build.add_argument("--debug", action="store_true")
    build.add_argument("--werror", action="store_true")
    build.set_defaults(handler=compile_sources)
    pack = commands.add_parser("pack", help="pack existing bgfx shaderc binaries from one or more hosts")
    pack.add_argument("--type", choices=STAGES, required=True)
    pack.add_argument("--binary", action="append", required=True, metavar="BACKEND=FILE")
    pack.add_argument("-o", "--output", type=Path, required=True)
    add_selection(pack, required=False)
    pack.set_defaults(handler=pack_binaries)
    inspect = commands.add_parser("inspect", help="validate a package and display its segment table")
    inspect.add_argument("input", type=Path)
    inspect.set_defaults(handler=inspect_package)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = create_parser().parse_args(argv)
    try:
        args.handler(args)
    except (ScbpError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
