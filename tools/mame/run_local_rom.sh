#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 MAME_SOURCE_DIR LOCAL_ROM_PATH [INSTRUCTION_BUDGET]" >&2
  exit 2
fi

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
mame_source="$(cd "$1" && pwd)"
rom_path="$(realpath "$2")"
instruction_budget="${3:-128}"
run_dir="$mame_source/research_vortexexec"

python3 - "$repo_root/tools/rom_inspector/rom_image.py" "$rom_path" <<'PY'
import importlib.util
import sys
from pathlib import Path
module_path, rom_path = map(Path, sys.argv[1:])
spec = importlib.util.spec_from_file_location("rom_image", module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.validate_rom(rom_path)
print(f"validated local ROM: {rom_path}")
PY

python3 "$repo_root/tools/mame/prepare_mame.py" "$mame_source"
mkdir -p "$mame_source/roms/vortexexec" "$run_dir"
cp "$rom_path" "$mame_source/roms/vortexexec/vortex.bin"

cd "$mame_source"
make -j2 SUBTARGET=tiny USE_QTDEBUG=0
binary="$mame_source/build/linux_gcc/bin/x64/Release/mame_tiny"
if [[ ! -x "$binary" ]]; then
  echo "MAME build did not produce $binary" >&2
  exit 1
fi

for run in a b; do
  cat > "$run_dir/debugger-$run.script" <<EOF
trace $run_dir/trace-$run.log,,noloop,{tracelog "AX0=%04X AX1=%04X AY0=%04X AY1=%04X AR=%08X MSTAT=%02X SSTAT=%02X IMASK=%04X I0=%04X I1=%04X I2=%04X I3=%04X cycles=%d total=%d",ax0,ax1,ay0,ay1,ar,mstat,sstat,imask,i0,i1,i2,i3,lastinstructioncycles,totalcycles}
step $instruction_budget
trace off
traceflush
print pc,ax0,ax1,ay0,ay1,ar,totalcycles
quit
EOF
  "$binary" vortexexec -rompath "$mame_source/roms" -video none -sound none -skip_gameinfo \
    -debug -debugscript "$run_dir/debugger-$run.script" -log -logfile "$run_dir/mame-$run.log"
done

cmp "$run_dir/trace-a.log" "$run_dir/trace-b.log"
if ! grep -q 'UNMAPPED external DM write PC=0053 address=1F80' "$run_dir/mame-a.log"; then
  echo "expected first unmapped hardware write at PC=0053, DM=1F80 was not logged" >&2
  exit 1
fi

echo "deterministic trace: $run_dir/trace-a.log"
echo "MAME log: $run_dir/mame-a.log"
