#!/usr/bin/env bash
# Build and install DMTCP into $HOME (no sudo required) so it matches the
# version built into the container's %post. Requires gcc/g++/make to
# already be available (e.g. `module load gcc` first if your cluster
# gates them behind a module).
set -euo pipefail

DMTCP_VERSION=4.2.0
PREFIX="$HOME/.local/dmtcp-${DMTCP_VERSION}"
SRC_DIR="$HOME/src/dmtcp-${DMTCP_VERSION}"

for tool in gcc g++ make curl tar; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: '$tool' not found on PATH - load it (e.g. module load gcc) and re-run" >&2
        exit 1
    fi
done

mkdir -p "$SRC_DIR"
cd "$SRC_DIR"

if [[ ! -f configure ]]; then
    echo "Downloading DMTCP ${DMTCP_VERSION} source..."
    curl -fL "https://github.com/dmtcp/dmtcp/archive/refs/tags/v${DMTCP_VERSION}.tar.gz" -o dmtcp.tar.gz
    tar --strip-components=1 -xzf dmtcp.tar.gz
fi

echo "Configuring (prefix: $PREFIX)..."
./configure --prefix="$PREFIX"

echo "Building..."
make -j"$(nproc)"

echo "Installing..."
make install

echo
echo "DMTCP ${DMTCP_VERSION} installed to $PREFIX"
echo "dmtcp/env.sh already points DMTCP_HOST_PREFIX here - no further action needed"
echo "unless you changed the version, in which case update DMTCP_VERSION in both"
echo "this script and the %post section of the .def file, then rebuild both sides."
