#!/usr/bin/env bash
# Main entry point for asyncga4-run.service.
#
# On every start (fresh boot, crash recovery, or manual restart) this:
#   1. waits for the DMTCP coordinator to be reachable,
#   2. tries dmtcp_restart from the newest checkpoint, falling back to
#      older generations if a checkpoint is corrupt,
#   3. if there is no usable checkpoint at all, does a fresh dmtcp_launch.
#
# systemd's Restart=on-failure (see systemd/asyncga4-run.service) is what
# actually re-invokes this script after a crash; this script's job is just
# to always resume from the best available state when it's re-invoked.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

CONTAINER_CKPT_DIR="/opt/asyncga4/ckpt"
CONTAINER_WORK_DIR="/opt/asyncga4/run"

log() { echo "$(date -Is) $*"; }

wait_for_coordinator() {
    local i
    for i in $(seq 1 30); do
        if (exec 3<>"/dev/tcp/${DMTCP_COORD_HOST}/${DMTCP_COORD_PORT}") 2>/dev/null; then
            exec 3<&- 3>&- 2>/dev/null || true
            return 0
        fi
        sleep 1
    done
    return 1
}

mkdir -p "$CKPT_DIR" "$WORK_DIR"

log "waiting for dmtcp coordinator at ${DMTCP_COORD_HOST}:${DMTCP_COORD_PORT}"
if ! wait_for_coordinator; then
    log "coordinator not reachable after 30s, aborting (systemd will retry)"
    exit 1
fi

BIND_ARGS=(--bind "$CKPT_DIR:$CONTAINER_CKPT_DIR" --bind "$WORK_DIR:$CONTAINER_WORK_DIR")

# Try restart from newest to oldest surviving checkpoint before falling
# back to a fresh launch.
mapfile -t CKPTS < <(ls -t "$CKPT_DIR"/ckpt_*.dmtcp 2>/dev/null || true)

for ckpt in "${CKPTS[@]}"; do
    ckpt_in_container="$CONTAINER_CKPT_DIR/$(basename "$ckpt")"
    log "attempting restart from checkpoint: $(basename "$ckpt")"
    if "$SINGULARITY_BIN" exec "${BIND_ARGS[@]}" "$SIF_PATH" \
        env DMTCP_COORD_HOST="$DMTCP_COORD_HOST" DMTCP_COORD_PORT="$DMTCP_COORD_PORT" \
            DMTCP_CHECKPOINT_DIR="$CONTAINER_CKPT_DIR" \
        dmtcp_restart --ckptdir "$CONTAINER_CKPT_DIR" "$ckpt_in_container"
    then
        log "process exited cleanly after restart"
        exit 0
    fi
    log "restart from $(basename "$ckpt") failed, trying next older checkpoint (if any)"
done

log "no usable checkpoint, launching fresh"
exec "$SINGULARITY_BIN" exec "${BIND_ARGS[@]}" "$SIF_PATH" \
    env DMTCP_COORD_HOST="$DMTCP_COORD_HOST" DMTCP_COORD_PORT="$DMTCP_COORD_PORT" \
        DMTCP_CHECKPOINT_DIR="$CONTAINER_CKPT_DIR" DMTCP_CHECKPOINT_INTERVAL=0 \
    dmtcp_launch --ckptdir "$CONTAINER_CKPT_DIR" \
    /usr/local/bin/AsynchronousGA4CL "${PROGRAM_ARGS[@]}"
