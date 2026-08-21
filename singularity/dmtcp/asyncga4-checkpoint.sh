#!/usr/bin/env bash
# Invoked every 10 minutes by asyncga4-checkpoint.timer. Asks the running
# coordinator for a checkpoint, confirms a genuinely new checkpoint file
# landed, then prunes old generations beyond KEEP_GENERATIONS.
#
# If no process is currently attached to the coordinator (e.g. between a
# crash and the next restart), the checkpoint request just fails - that is
# expected and NOT treated as an error; the timer tries again next cycle.
set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

log() { echo "$(date -Is) $*"; }

before_newest=$(ls -t "$CKPT_DIR"/ckpt_*.dmtcp 2>/dev/null | head -n1 || true)
before_mtime=0
[[ -n "$before_newest" ]] && before_mtime=$(stat -c %Y "$before_newest" 2>/dev/null || echo 0)

log "requesting checkpoint from coordinator ${DMTCP_COORD_HOST}:${DMTCP_COORD_PORT}"
if ! dmtcp_command --coord-host "$DMTCP_COORD_HOST" --coord-port "$DMTCP_COORD_PORT" --checkpoint; then
    log "checkpoint request failed (no client attached right now?) - will retry next interval"
    exit 0
fi

# Confirm a strictly newer checkpoint file actually landed before pruning
# anything - never delete on an unconfirmed checkpoint.
newest=""
for _ in $(seq 1 60); do
    newest=$(ls -t "$CKPT_DIR"/ckpt_*.dmtcp 2>/dev/null | head -n1 || true)
    if [[ -n "$newest" ]]; then
        newest_mtime=$(stat -c %Y "$newest" 2>/dev/null || echo 0)
        (( newest_mtime > before_mtime )) && break
    fi
    sleep 1
done

newest_mtime=0
[[ -n "$newest" ]] && newest_mtime=$(stat -c %Y "$newest" 2>/dev/null || echo 0)

if [[ -z "$newest" ]] || (( newest_mtime <= before_mtime )); then
    log "no new checkpoint file detected within 60s, skipping retention cleanup this round"
    exit 0
fi

log "new checkpoint confirmed: $(basename "$newest")"

mapfile -t all_ckpts < <(ls -t "$CKPT_DIR"/ckpt_*.dmtcp 2>/dev/null)
if (( ${#all_ckpts[@]} > KEEP_GENERATIONS )); then
    for f in "${all_ckpts[@]:$KEEP_GENERATIONS}"; do
        log "pruning old checkpoint: $(basename "$f")"
        rm -f -- "$f"
    done
fi
