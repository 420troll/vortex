"""Add the local Vortex execution driver to a pinned MAME source checkout."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path

MAME_COMMIT = "d19b5702bdbd1e6c14ecd522e9236154468fc4ef"


def prepare(mame: Path, driver: Path) -> None:
    actual = subprocess.check_output(["git", "-C", str(mame), "rev-parse", "HEAD"], text=True).strip()
    if actual != MAME_COMMIT:
        raise ValueError(f"expected MAME {MAME_COMMIT}, got {actual}")

    destination = mame / "src/mame/vortex/vortex_exec.cpp"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(driver, destination)

    # Replace the stock tiny profile (which references many unrelated drivers)
    # with a single-driver profile so a sparse MAME checkout is sufficient.
    tiny_lua = mame / "scripts/target/mame/tiny.lua"
    tiny_lua.write_text('''-- Local Vortex ADSP-2105 execution harness.
CPUS["ADSP21XX"] = true

function createProjects_mame_tiny(_target, _subtarget)
    project("mame_tiny")
    targetsubdir(_target .. "_" .. _subtarget)
    kind(LIBTYPE)
    uuid(os.uuid("drv-mame-tiny"))
    addprojectflags()
    precompiledheaders_novs()
    includedirs {
        MAME_DIR .. "src/osd", MAME_DIR .. "src/emu", MAME_DIR .. "src/devices",
        MAME_DIR .. "src/mame/shared", MAME_DIR .. "src/lib", MAME_DIR .. "src/lib/util",
        MAME_DIR .. "3rdparty", GEN_DIR .. "mame/layout",
    }
    files { MAME_DIR .. "src/mame/vortex/vortex_exec.cpp" }
end

function linkProjects_mame_tiny(_target, _subtarget)
    links { "mame_tiny" }
end
''')

    tiny_lst = mame / "src/mame/tiny.lst"
    tiny_lst.write_text("@source:vortex/vortex_exec.cpp\nvortexexec\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mame_source", type=Path)
    parser.add_argument("--driver", type=Path, default=Path(__file__).with_name("vortex_exec.cpp"))
    args = parser.parse_args()
    prepare(args.mame_source.resolve(), args.driver.resolve())


if __name__ == "__main__":
    main()
