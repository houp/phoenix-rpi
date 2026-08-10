#!/usr/bin/env bash
#
# Launch a headless NetQuake dedicated server on the host for Pi-client
# multiplayer testing (KNOWN-ISSUES #68: Phoenix Quake MP hangs at LOADING).
#
# Built from external/quakespasm (same codebase as the Phoenix client, so the
# NetQuake protocol matches). Runs with the SDL dummy video/audio drivers so it
# needs no display, and binds UDP 26000 on 0.0.0.0 -- reachable from the Pi over
# the netboot network at the host IP (10.42.0.1 by default).
#
# Usage:  ./scripts/quake-mp-server.sh [map] [protocol]      (default: start 15)
# Stop:   Ctrl-C, or kill the printed PID.
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$HERE/.." && pwd)"
QS="$REPO_ROOT/external/quakespasm/Quake/quakespasm"
QDIR="${QUAKE_BASEDIR:-$REPO_ROOT/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/rootfs-overlay/usr/share/quake}"
MAP="${1:-start}"
PROTO="${2:-15}"

if [ ! -x "$QS" ]; then
	echo "quake-mp-server: building host quakespasm dedicated server..."
	make -C "$REPO_ROOT/external/quakespasm/Quake" USE_SDL2=1 \
		USE_CODEC_VORBIS=0 USE_CODEC_MP3=0 USE_CODEC_FLAC=0 USE_CODEC_OPUS=0 \
		USE_CODEC_MIKMOD=0 USE_CODEC_UMX=0 USE_CODEC_XMP=0 -j4
fi
if [ ! -f "$QDIR/id1/pak0.pak" ]; then
	echo "quake-mp-server: no id1/pak0.pak under $QDIR" >&2
	exit 1
fi

echo "quake-mp-server: id1=$QDIR/id1  map=$MAP  protocol=$PROTO  udp=0.0.0.0:26000"
echo "quake-mp-server: connect from the Pi with:  connect 10.42.0.1"
exec env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
	"$QS" -basedir "$QDIR" -dedicated 4 +sv_protocol "$PROTO" +map "$MAP"
