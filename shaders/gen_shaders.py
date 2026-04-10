import subprocess
from pathlib import Path
import re
import tempfile
import os

SLANG_BINARY = (
        Path(__file__).resolve().parent.parent
        / "engine"
        / "extern"
        / "slangc"
        / "slangc"
)

ROOT_DIR = Path(__file__).parent
OUT_FILE = Path(__file__).parent.parent / "engine" / "inc" / "shader_data.hpp"

SLANG_TARGET = "dxil"

SHADER_STAGE_MAP = {
    "compute": "cs",
    "vertex": "vs",
    "pixel": "ps",
    "geometry": "gs",
    "hull": "hs",
    "domain": "ds",
    "mesh": "ms",
    "amplification": "as",
}

VARIANT_PREFIX = "VARIANT_"

def detect_shader_stages(shader_path: Path) -> list[tuple[str, str]]:
    text = shader_path.read_text(encoding="utf-8", errors="ignore")

    pattern = r'\[shader\s*\(\s*"(\w+)"\s*\)\]\s*\n\s*(?:export\s+)?[\w\d_]+\s+(\w+)\s*\('
    matches = re.finditer(pattern, text)

    stages = []
    for match in matches:
        stage_name = match.group(1).lower()
        entry_point = match.group(2)

        if stage_name not in SHADER_STAGE_MAP:
            print(f"Warning: Unsupported shader stage '{stage_name}' in {shader_path}, skipping")
            continue

        stages.append((entry_point, SHADER_STAGE_MAP[stage_name]))

    if not stages:
        raise RuntimeError(
            f"No [shader(\"...\")] attributes found in {shader_path}"
        )

    return stages

def detect_variants(shader_path: Path) -> list[str]:
    """
    Returns a list of variant names (e.g. ['OPAQUE', 'TRANSPARENT']) if any
    #define VARIANT_* lines are found, otherwise returns an empty list (no variants).
    """
    text = shader_path.read_text(encoding="utf-8", errors="ignore")
    pattern = r'^\s*#define\s+(VARIANT_\w+)',
    matches = re.findall(r'^\s*#define\s+(VARIANT_\w+)', text, re.MULTILINE)
    return matches  # e.g. ['VARIANT_OPAQUE', 'VARIANT_TRANSPARENT']

def strip_variant_defines(text: str) -> str:
    """Remove all #define VARIANT_* lines from shader source."""
    return re.sub(r'^\s*#define\s+VARIANT_\w+[^\n]*\n?', '', text, flags=re.MULTILINE)

def compile_to_dxil(
        shader_path: Path,
        entry_point: str,
        stage: str,
        defines: list[str] | None = None,
) -> bytes:
    profile = f"{stage}_6_6"

    # If we have variant defines, we need a temp file with the #define VARIANT_* lines stripped
    # so that only the -D flag controls which variant is active.
    if defines:
        original_text = shader_path.read_text(encoding="utf-8", errors="ignore")
        stripped_text = strip_variant_defines(original_text)
        tmp = tempfile.NamedTemporaryFile(
            mode="w",
            suffix=shader_path.suffix,
            dir=shader_path.parent,
            delete=False,
            encoding="utf-8",
        )
        try:
            tmp.write(stripped_text)
            tmp.close()
            source_path = Path(tmp.name)
            dxil_path = shader_path.with_suffix(f".{entry_point}.dxil.tmp")
            _run_slangc(source_path, entry_point, profile, dxil_path, defines)
        finally:
            os.unlink(tmp.name)
    else:
        dxil_path = shader_path.with_suffix(f".{entry_point}.dxil.tmp")
        _run_slangc(shader_path, entry_point, profile, dxil_path, defines)

    data = dxil_path.read_bytes()
    dxil_path.unlink(missing_ok=True)
    return data

def _run_slangc(
        source_path: Path,
        entry_point: str,
        profile: str,
        out_path: Path,
        defines: list[str] | None,
):
    cmd = [
        str(SLANG_BINARY),
        str(source_path),
        "-entry", entry_point,
        "-target", "dxil",
        "-profile", profile,
        "-matrix-layout-column-major",
        "-g",
        "-O0",
        "-o", str(out_path),
    ]
    if defines:
        for d in defines:
            cmd += ["-D", d]

    label = f"{source_path.name} [{entry_point} - {profile}]"
    if defines:
        label += f" defines=[{', '.join(defines)}]"
    print(f"Compiling: {label}")
    subprocess.run(cmd, check=True)

def main():
    arrays = []

    for file in ROOT_DIR.rglob("*"):
        if not file.is_file():
            continue
        if file.suffix == ".py":
            continue

        base_name = file.stem.replace("-", "_")

        try:
            stages = detect_shader_stages(file)
        except RuntimeError as e:
            print(f"Skipping {file}: {e}")
            continue

        variants = detect_variants(file)

        for entry_point, stage in stages:
            if variants:
                # Compile once per variant, passing only that variant's define
                for variant in variants:
                    # e.g. VARIANT_OPAQUE -> 'opaque'
                    variant_suffix = variant[len(VARIANT_PREFIX):].lower()
                    # e.g. geom_opaque_mesh_main_code
                    var_name = f"{base_name}_{variant_suffix}_{entry_point}_code"

                    try:
                        bytecode = compile_to_dxil(file, entry_point, stage, defines=[variant])
                    except subprocess.CalledProcessError as e:
                        print(f"Failed: {file} [{entry_point}] variant={variant}: {e}")
                        continue

                    size = len(bytecode)
                    bytes_cpp = ", ".join(f"0x{b:02x}" for b in bytecode)
                    arrays.append(
                        f"inline constexpr std::array<uint8_t, {size}> {var_name} = {{\n"
                        f"  {bytes_cpp}\n"
                        f"}};"
                    )
            else:
                # No variants — compile as before
                var_name = f"{base_name}_{entry_point}_code"
                try:
                    bytecode = compile_to_dxil(file, entry_point, stage)
                except subprocess.CalledProcessError as e:
                    print(f"Failed: {file} [{entry_point}]: {e}")
                    continue

                size = len(bytecode)
                bytes_cpp = ", ".join(f"0x{b:02x}" for b in bytecode)
                arrays.append(
                    f"inline constexpr std::array<uint8_t, {size}> {var_name} = {{\n"
                    f"  {bytes_cpp}\n"
                    f"}};"
                )

    OUT_FILE.write_text(
        "#pragma once\n"
        + "\n\n".join(arrays)
    )

    print(f"Written: {OUT_FILE}")

if __name__ == "__main__":
    main()