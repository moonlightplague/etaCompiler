set -euo pipefail

DIR="tests/pa6/extra_testcases"
MODE="eta"
ETA_FILE="sort"
OUTPUT="output/"
DIAGNOSE="diagnose/"
BINARY="${OUTPUT}/binary"
TEST_CASE="${DIR}/${ETA_FILE}.input"
INTERPRET=false
GRAPH=false

echo "Running Compilation: "
"./etac" \
    -sourcepath "$DIR" \
    -libpath "$DIR" \
    -D "$DIAGNOSE" \
    -d "$OUTPUT" \
    --optcfg final \
    --optir final \
    "${ETA_FILE}.${MODE}"

echo "Running Execution: "
if [ -f "$TEST_CASE" ]; then
    /usr/bin/time -f " User Time: %U \n System Time: %S \n Max RSS: %M KB \n Major (requiring I/O) page faults: %F \n Minor (reclaiming a frame) page faults: %R" "$BINARY" < "$TEST_CASE"
else
    /usr/bin/time -f " User Time: %U \n System Time: %S \n Max RSS: %M KB \n Major (requiring I/O) page faults: %F \n Minor (reclaiming a frame) page faults: %R" "$BINARY"
fi
#visualize dot files
if $GRAPH; then
    if command -v dot >/dev/null 2>&1; then
        shopt -s nullglob
        for dot_file in "${OUTPUT}"*.dot; do
            dot -Tpng "$dot_file" -o "${dot_file%.dot}.png"
        done
        shopt -u nullglob
    else
        echo "Graphviz dot not found; skipping CFG visualization." >&2
    fi
fi

if $INTERPRET; then
echo ""
echo "Running Interpretation for Verification: "
time "./etac" \
    --irrun \
    -sourcepath "$DIR" \
    -libpath "$DIR" \
    -D "$DIAGNOSE" \
    -d "$OUTPUT" \
    "${ETA_FILE}.${MODE}"
fi

rm *.dot
rm *.ir
