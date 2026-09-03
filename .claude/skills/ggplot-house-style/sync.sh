#!/usr/bin/env bash
# Copy this skill from its canonical home (~/skillhub) into a consuming repo's
# .claude/skills/ dir. Plain copy (not a symlink) so the skill survives
# `git clone` on other machines.
#
# Usage:  bash sync.sh /path/to/target-repo
#         bash sync.sh                       # defaults to the Hermes repo
set -euo pipefail

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NAME="$(basename "$SELF_DIR")"
TARGET_REPO="${1:-/home/rahbera/thesis/Hermes}"
DEST="$TARGET_REPO/.claude/skills/$NAME"

if [ ! -d "$TARGET_REPO/.claude" ]; then
  echo "FAIL: $TARGET_REPO has no .claude/ dir — is it the right repo?" >&2
  exit 1
fi

mkdir -p "$DEST"
# --delete keeps the copy exact; exclude VCS noise.
rsync -a --delete --exclude '.git' "$SELF_DIR/" "$DEST/"
echo "Synced $NAME -> $DEST"
