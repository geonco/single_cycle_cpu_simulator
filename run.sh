# Usage: ./run.sh [imem_file] [dmem_file]   (defaults: imem.mem dmem.mem)
set -e
cd "$(dirname "$0")"

IMEM="${1:-imem.mem}"
DMEM="${2:-dmem.mem}"

gcc -Wall -o rv32i_single rv32i_single.c
./rv32i_single "$IMEM" "$DMEM"

echo "=== DONE: report.txt (RF + DMEM dump) written ==="