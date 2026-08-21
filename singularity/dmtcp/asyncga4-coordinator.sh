#!/usr/bin/env bash
# Long-running DMTCP coordinator. Started by asyncga4-coordinator.service
# and left running - it survives individual restarts of the compute
# process (asyncga4-run.service) so checkpoint history keeps accumulating
# across crash/restart cycles.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

mkdir -p "$CKPT_DIR"

exec dmtcp_coordinator --coord-port "$DMTCP_COORD_PORT" --ckptdir "$CKPT_DIR" --quiet
