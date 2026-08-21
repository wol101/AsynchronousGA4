#!/usr/bin/env bash
# Shared configuration for the AsynchronousGA4CL DMTCP checkpoint/restart
# setup. Edit the values below for your host. This file is sourced by
# asyncga4-coordinator.sh, asyncga4-run.sh and asyncga4-checkpoint.sh - it
# is the single place to change paths/ports.
#
# The systemd unit files reference the scripts by absolute path
# (%h/asyncga4/dmtcp/...), so if you move this whole "dmtcp" folder
# somewhere other than $HOME/asyncga4/dmtcp, update the ExecStart= lines
# in dmtcp/systemd/*.service to match.

# Absolute path to the built .sif container image.
export SIF_PATH="$HOME/asyncga4/asynchronousga4cl_container.sif"

# Host-side directories that get bind-mounted into the container at
# /opt/asyncga4/ckpt and /opt/asyncga4/run (fixed paths baked into the
# image - see the %post section of the .def file). These must live
# somewhere that survives a reboot, i.e. NOT /tmp or a scratch filesystem
# that gets wiped.
export CKPT_DIR="$HOME/asyncga4/ckpt"
export WORK_DIR="$HOME/asyncga4/run"

# DMTCP coordinator location. 127.0.0.1 works because Singularity/Apptainer
# does not isolate the network namespace by default, so the container sees
# the same loopback as the host. Pick a port unlikely to collide with other
# users on a shared node.
export DMTCP_COORD_HOST="127.0.0.1"
export DMTCP_COORD_PORT="17779"

# Host-side DMTCP install (built by install_dmtcp_host.sh). Must be the
# same DMTCP_VERSION as the one built into the container's %post.
export DMTCP_HOST_PREFIX="$HOME/.local/dmtcp-4.2.0"
export PATH="$DMTCP_HOST_PREFIX/bin:$PATH"

# How singularity is invoked. systemd --user services do NOT go through
# your login shell or `module load`, so if `singularity` isn't on the bare
# PATH here, set this to the absolute path (`which singularity` with the
# module loaded) or add the module's bin dir to PATH above.
export SINGULARITY_BIN="${SINGULARITY_BIN:-singularity}"

# How many checkpoint generations to keep on disk (most recent first).
# Restart tries them newest-to-oldest, falling back if one is corrupt.
export KEEP_GENERATIONS=3

# Command-line options passed to AsynchronousGA4CL on the *first* launch
# only - a dmtcp_restart resumes the exact process memory image, so these
# are not re-parsed on restart. EDIT THIS for your actual run.
export PROGRAM_ARGS=(--population 200 --generations 100000)
