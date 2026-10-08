#!/bin/sh
# Build all four id maps for one older install into the server's res/compat/<profile>/ (run from anywhere).
#   sh build_maps.sh <install root> <cache tag> <profile> [high shift: zone files 256+ this many ids below PC]
set -e
cd "$(dirname "$0")"
export XI_OLD="$1" XI_OLD_TAG="$2" XI_OLD_HIGH_SHIFT="${4:-0}"
OUT="$(cd "$(dirname "$0")/../.." && pwd)/res/compat/$3"
mkdir -p "$OUT"
python dialog_map.py "$OUT/dialog_map.bin"
python entity_map.py "$OUT/entity_map.bin"
python item_map.py "$OUT/items.bin"
python event_map.py "$OUT/events.bin"
echo "maps for $3 done"
