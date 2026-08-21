# DMTCP checkpoint/restart for AsynchronousGA4CL

Runs `AsynchronousGA4CL` inside the Singularity container under [DMTCP](https://github.com/dmtcp/dmtcp),
checkpointing every 10 minutes to a host directory that survives container
restarts, and automatically resuming from the latest checkpoint after a
crash or a full node reboot.

## How it fits together

```
host (systemd --user, always running)
  asyncga4-coordinator.service ---> dmtcp_coordinator (127.0.0.1:17779)
                                          ^
  asyncga4-run.service ---> asyncga4-run.sh ---> singularity exec --bind ... container.sif
                                                    env DMTCP_COORD_HOST=... \
                                                    dmtcp_launch|dmtcp_restart AsynchronousGA4CL
                                          ^
  asyncga4-checkpoint.timer (every 10 min)
    -> asyncga4-checkpoint.service -> asyncga4-checkpoint.sh -> dmtcp_command --checkpoint
```

- The **coordinator** runs on the host, outside the container, and stays up
  independently of the compute process. Singularity does not isolate the
  network namespace by default, so `127.0.0.1` inside the container reaches
  the host's coordinator directly.
- The **compute process** runs inside the container via `dmtcp_launch`
  (first run) or `dmtcp_restart` (every subsequent run). `dmtcp_restart`
  must run inside the container because it recreates the checkpointed
  process in the same filesystem/library environment it was checkpointed
  in.
- **Checkpoint files** live on the host at `$CKPT_DIR` (default
  `~/asyncga4/ckpt`) and are bind-mounted into the container at the fixed
  path `/opt/asyncga4/ckpt`, so they persist across container invocations
  and host reboots.
- `asyncga4-run.service` uses `Restart=on-failure`, so when the compute
  process crashes, systemd re-invokes `asyncga4-run.sh`, which finds the
  newest checkpoint and resumes from it. The same script also runs at boot
  (once linger is enabled - see below), so a full node reboot is handled
  the same way.

## Prerequisite: the binary must be dynamically linked

DMTCP injects its checkpoint runtime via `LD_PRELOAD`, which requires a
dynamic linker. A fully static binary has no `PT_INTERP`/`.dynamic` section
and cannot be checkpointed at all - confirmed against the current build:

```
$ file AsynchronousGA4CL
...statically linked...
$ readelf -l AsynchronousGA4CL | grep INTERP     # (nothing)
$ readelf -d AsynchronousGA4CL                    # "There is no dynamic section in this file."
```

Rebuild it without `-static` (you can keep `-static-libgcc
-static-libstdc++` if you still want those statically linked for
portability - only libc/ld need to be dynamic). After rebuilding, confirm:

```
$ file AsynchronousGA4CL
...dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2...
```

Do this **before** attempting any of the steps below - everything else
depends on it.

## One-time setup

### 1. Build the container

The `.def` file now builds DMTCP 4.2.0 from source into the image
(`%post`), pinned to the same version you'll install on the host in step 2.
Build as usual:

```bash
sudo singularity build asynchronousga4cl_container.sif asynchronousga4cl_static_singularity.def
```

Copy the resulting `.sif` to the host/node this will run on, at the path
you'll set as `SIF_PATH` in `env.sh` (default `~/asyncga4/asynchronousga4cl_container.sif`).

### 2. Install DMTCP on the host (no sudo)

```bash
mkdir -p ~/asyncga4
cp -r dmtcp ~/asyncga4/dmtcp   # or clone/copy this whole folder there
~/asyncga4/dmtcp/install_dmtcp_host.sh
```

This builds DMTCP 4.2.0 from source into `~/.local/dmtcp-4.2.0`. If
`gcc`/`g++`/`make` aren't already on `PATH`, load them via your cluster's
module system first (e.g. `module load gcc`) - most HPC systems provide a
compiler toolchain to unprivileged users even without root.

### 3. Configure `env.sh`

Edit `~/asyncga4/dmtcp/env.sh`:

- `SIF_PATH` - path to the `.sif` from step 1
- `CKPT_DIR` / `WORK_DIR` - host directories to bind into the container
  (must be on storage that survives a reboot, not `/tmp`)
- `DMTCP_COORD_PORT` - change if 17779 collides with something else on a
  shared node
- `SINGULARITY_BIN` - **important on HPC**: systemd `--user` services do
  not go through your login shell or `module load`. If `singularity` isn't
  found on the bare `PATH`, either set this to the absolute path (run
  `which singularity` with the module loaded) or add the module's bin
  directory to the `PATH` line above it.
- `PROGRAM_ARGS` - the actual command-line options for `AsynchronousGA4CL`
  (only used on the very first launch; a restart resumes the exact process
  memory image, so these aren't re-parsed after that)

### 4. Manual smoke test (do this before wiring up systemd)

Confirm DMTCP can actually checkpoint/restart this binary before trusting
it to run unattended:

```bash
source ~/asyncga4/dmtcp/env.sh
mkdir -p "$CKPT_DIR" "$WORK_DIR"

# terminal 1: coordinator in the foreground
dmtcp_coordinator --coord-port "$DMTCP_COORD_PORT" --ckptdir "$CKPT_DIR"

# terminal 2: launch under DMTCP
singularity exec --bind "$CKPT_DIR:/opt/asyncga4/ckpt" --bind "$WORK_DIR:/opt/asyncga4/run" \
    "$SIF_PATH" \
    env DMTCP_COORD_HOST=127.0.0.1 DMTCP_COORD_PORT="$DMTCP_COORD_PORT" \
        DMTCP_CHECKPOINT_DIR=/opt/asyncga4/ckpt DMTCP_CHECKPOINT_INTERVAL=0 \
    dmtcp_launch --ckptdir /opt/asyncga4/ckpt /usr/local/bin/AsynchronousGA4CL "${PROGRAM_ARGS[@]}"

# terminal 3: trigger a checkpoint, then confirm a ckpt_*.dmtcp file appeared
dmtcp_command --coord-host 127.0.0.1 --coord-port "$DMTCP_COORD_PORT" --checkpoint
ls -la "$CKPT_DIR"

# kill the process in terminal 2 (Ctrl-C or kill its PID), then restart it
# from the checkpoint you just took:
singularity exec --bind "$CKPT_DIR:/opt/asyncga4/ckpt" --bind "$WORK_DIR:/opt/asyncga4/run" \
    "$SIF_PATH" \
    env DMTCP_COORD_HOST=127.0.0.1 DMTCP_COORD_PORT="$DMTCP_COORD_PORT" \
    dmtcp_restart --ckptdir /opt/asyncga4/ckpt /opt/asyncga4/ckpt/ckpt_*.dmtcp
```

If the restarted process picks up exactly where the checkpoint left off,
you're good to automate it. If `dmtcp_launch` fails immediately, re-check
the binary is dynamically linked (previous section) - that's the most
common cause of failure here.

Stop the coordinator (Ctrl-C in terminal 1) before moving on.

### 5. Install the systemd user units

```bash
mkdir -p ~/.config/systemd/user
cp ~/asyncga4/dmtcp/systemd/*.service ~/asyncga4/dmtcp/systemd/*.timer ~/.config/systemd/user/
systemctl --user daemon-reload
```

### 6. Enable linger (so services survive logout and start at boot)

By default, `systemd --user` units stop when you log out, and don't start
automatically on reboot unless the user has an active session. `linger`
fixes both:

```bash
loginctl enable-linger "$USER"
loginctl show-user "$USER" | grep Linger   # should print Linger=yes
```

This normally does not require root. If it fails with a permission error,
ask your cluster admin to run `sudo loginctl enable-linger <you>` once.

### 7. Enable and start everything

```bash
systemctl --user enable --now asyncga4-coordinator.service
systemctl --user enable --now asyncga4-run.service
systemctl --user enable --now asyncga4-checkpoint.timer
```

## Verifying it works

```bash
# service status
systemctl --user status asyncga4-coordinator.service asyncga4-run.service asyncga4-checkpoint.timer

# live logs
journalctl --user -u asyncga4-run.service -f
journalctl --user -u asyncga4-checkpoint.service -f

# checkpoints accumulating (should see a new one roughly every 10 min,
# old generations pruned beyond KEEP_GENERATIONS)
watch ls -la ~/asyncga4/ckpt
```

**Crash-recovery test:** find the PID of the actual `AsynchronousGA4CL`
process inside the container (`journalctl --user -u asyncga4-run.service`
or `ps -ef | grep AsynchronousGA4CL`) and `kill -9` it. Within
`RestartSec=30`, `asyncga4-run.service` should relaunch and
`asyncga4-run.sh` should log that it's restarting from the newest
checkpoint rather than starting fresh.

**Reboot test:** reboot the node, log back in (or just check
`systemctl --user status` after linger has kicked in), and confirm the
three units came back up on their own and the run service resumed from
checkpoint.

## Known limitations

- If `asyncga4-coordinator.service` itself crashes/restarts while the
  compute process is still running, the running process's connection to
  the old coordinator is gone and checkpointing stops until
  `asyncga4-run.service` itself is next restarted (which reconnects to the
  new coordinator). The coordinator is deliberately kept simple with no
  live-client work to do, so this should be rare in practice, but it's a
  gap - there's no hot coordinator-migration in this setup.
- `Restart=on-failure` means a clean/intentional exit (e.g. the GA run
  finishing) is not auto-relaunched, matching "restart on crash". If you
  want it to also relaunch after *any* exit, change to `Restart=always` in
  `asyncga4-run.service` - but note that will also re-run from the last
  checkpoint after a deliberate finish, which is usually not what you want
  for a job that's designed to terminate.
- Checkpoint image size scales with the process's resident memory, and a
  10-minute interval on a long-running job can add up; `KEEP_GENERATIONS`
  (default 3) bounds disk usage but raise/lower it in `env.sh` based on
  image size and available disk.
- DMTCP version must match exactly between the host install and the
  container's `%post` (`DMTCP_VERSION` in both `install_dmtcp_host.sh` and
  the `.def` file). If you ever bump one, bump the other and rebuild both.

## Troubleshooting

- **`dmtcp_launch: command not found` under systemd but works manually** -
  almost always a `PATH`/`SINGULARITY_BIN` issue since systemd `--user`
  doesn't source your shell profile or run `module load`. Set full paths
  in `env.sh`.
- **`asyncga4-run.service` keeps restarting immediately** - check
  `journalctl --user -u asyncga4-run.service` for the actual singularity/
  dmtcp error; a common cause is the coordinator not being up yet (check
  `asyncga4-coordinator.service` status) or a stale/corrupt checkpoint
  (the script already falls back through older generations automatically,
  but if all of them are bad it falls through to a fresh launch).
- **No checkpoints ever appear** - confirm `asyncga4-checkpoint.timer` is
  active (`systemctl --user list-timers`) and check
  `journalctl --user -u asyncga4-checkpoint.service`; "checkpoint request
  failed" there most often means the coordinator has no client attached
  right now (process between crash and restart) - not itself a bug.
- **Services don't start after reboot** - re-check `loginctl show-user
  "$USER"` shows `Linger=yes`.
