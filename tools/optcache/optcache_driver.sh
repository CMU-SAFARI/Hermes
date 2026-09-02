#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "$SCRIPT_DIR/optcache_driver $*"
"$SCRIPT_DIR/optcache_driver" "$@"
