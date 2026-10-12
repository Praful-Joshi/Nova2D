#!/usr/bin/env bash
# Launches N local Tag Peers (IDs 1..N), each in its own window, and waits for them.
# Ctrl-C stops them all. Extra arguments are passed to every Peer.
#
#   scripts/run_tag.sh 3
#   NOVA2D_TAG=path/to/nova2d_tag scripts/run_tag.sh 2
set -euo pipefail

players="${1:-2}"
shift || true
if ! [[ "$players" =~ ^[1-4]$ ]]; then
    echo "usage: $0 N [nova2d_tag options...]   (N = number of Peers, 1-4)" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tag="${NOVA2D_TAG:-$repo_root/build/bin/nova2d_tag}"
if [[ ! -x "$tag" ]]; then
    echo "nova2d_tag not found at $tag; build it first (or set NOVA2D_TAG)" >&2
    exit 1
fi

pids=()
stop_all() {
    trap - INT TERM
    for pid in ${pids[@]+"${pids[@]}"}; do
        kill "$pid" 2>/dev/null || true
    done
    wait 2>/dev/null || true
}
trap 'stop_all; exit 130' INT TERM

for ((id = 1; id <= players; id++)); do
    "$tag" --id "$id" --players "$players" "$@" &
    pids+=("$!")
done

echo "Started $players Peer(s): ${pids[*]}. Press Ctrl-C to stop them all."
wait
