#!/bin/bash
set -e

BASE="/tmp/overlay-lab"
LOWER0="$BASE/lower0"

RUNS=(
  'echo "BIB1" > file1.txt'
  'echo "BIB2" > file1.txt'
  'rm file1.txt'
  'echo "BIB3" > file3.txt'
)

# Cleanup
sudo umount "$BASE/merged" 2>/dev/null || true
sudo rm -rf "$BASE"
mkdir -p "$LOWER0"

# Create base filesystem
echo "Base filesystem" > "$LOWER0/base.txt"

LOWERS=("$LOWER0")

for i in "${!RUNS[@]}"; do
  UPPER="$BASE/upper$i"
  WORK="$BASE/work$i"
  MERGED="$BASE/merged"

  mkdir -p "$UPPER" "$WORK" "$MERGED"

  # Build lowerdir string (newest first)
  LOWERDIR=$(printf ":%s" "${LOWERS[@]}")
  LOWERDIR=${LOWERDIR:1}

  sudo mount -t overlay overlay \
    -o lowerdir="$LOWERDIR",upperdir="$UPPER",workdir="$WORK" \
    "$MERGED"

  # Execute RUN inside this filesystem
  sudo bash -c "cd '$MERGED' && ${RUNS[$i]}"

  sudo umount "$MERGED"

  # Add this upper as a new lower layer
  LOWERS=("$UPPER" "${LOWERS[@]}")
done

echo "Build complete."
echo "Final layers (top → bottom):"
printf '%s\n' "${LOWERS[@]}"

