#!/bin/sh
# Extract the plugin summary CMake prints at the end of configuration, and
# compare it against the committed baseline.
#
# x-AMP has no test suite, so this is the main automated guard: a change in
# libqmmp can disable a distant plugin with no other visible symptom.
#
# Usage: ci/plugin-summary.sh <configure-log> [baseline]
#
# Exit status:
#   0  no plugin regressed (or no baseline yet -- see below)
#   1  at least one plugin went from enabled to disabled
#   2  usage error
#
# With no baseline present the script prints the current summary and exits 0,
# so the first CI run tells you what to commit as ci/plugins-baseline.txt
# rather than failing on a file nobody could have written yet.

set -eu

log=${1:-}
baseline=${2:-ci/plugins-baseline.txt}

if [ -z "$log" ] || [ ! -f "$log" ]; then
    echo "usage: $0 <configure-log> [baseline]" >&2
    exit 2
fi

current=$(mktemp)
trap 'rm -f "$current"' EXIT

# Summary lines look like "HTTP support .......................enabled".
# Collapse the leader so the baseline survives cosmetic realignment upstream,
# and sort so the diff does not depend on print order. The leader is matched
# as [ .,]{2,} rather than dots only: one upstream line is padded to just two
# dots ("Removable device detection (Windows) ..disabled") and another has a
# stray comma in it ("UDisks support ......,.......enabled").
sed -n 's/^\(.*[A-Za-z0-9)]\)[ .,]\{2,\}\(enabled\|disabled\)$/\1 = \2/p' "$log" \
    | sort > "$current"

if [ ! -s "$current" ]; then
    echo "error: no plugin summary found in $log" >&2
    echo "       (did cmake fail before printing it?)" >&2
    exit 2
fi

echo "Plugins enabled:  $(grep -c '= enabled$' "$current")"
echo "Plugins disabled: $(grep -c '= disabled$' "$current")"
echo

if [ ! -f "$baseline" ]; then
    echo "No baseline at $baseline -- nothing to compare against yet."
    echo "Commit the summary below as $baseline to turn on the check:"
    echo
    cat "$current"
    exit 0
fi

# A plugin that became available is fine and often just means the runner
# grew a dependency; only the other direction is a regression.
regressed=$(awk -F' = ' '
    NR == FNR { was[$1] = $2; next }
    { now[$1] = $2 }
    END {
        for (p in was)
            if (was[p] == "enabled" && now[p] != "enabled")
                print p
    }
' "$baseline" "$current" | sort)

gained=$(awk -F' = ' '
    NR == FNR { was[$1] = $2; next }
    { now[$1] = $2 }
    END {
        for (p in now)
            if (now[p] == "enabled" && was[p] != "enabled")
                print p
    }
' "$baseline" "$current" | sort)

if [ -n "$gained" ]; then
    echo "Newly enabled (not a failure):"
    echo "$gained" | sed 's/^/  + /'
    echo
fi

if [ -n "$regressed" ]; then
    echo "REGRESSION: these plugins were enabled in the baseline and are not now:"
    echo "$regressed" | sed 's/^/  - /'
    echo
    echo "Either a change disabled them, or the runner lost a dependency."
    echo "If the new state is correct, update $baseline."
    exit 1
fi

echo "No plugin regressions against $baseline."
