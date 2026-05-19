#!/usr/bin/env bash
#
# Build the Phoenix-RTOS aarch64 cross-toolchain on Linux. This is a
# multi-hour build — runs binutils-2.43 + gcc-14.2.0 with Phoenix's
# patches applied. Produces:
#
#   $REPO/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-{gcc,ld,as,...}
#
# After this finishes, scripts/rebuild-rpi4b-fast.sh on Linux can find
# the toolchain at its default path.
#
# Idempotent: if the toolchain is already built (aarch64-phoenix-gcc
# already on the expected path), exits 0 immediately.
#

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
sources_dir="${repo_root}/sources"
toolchain_dir="${repo_root}/.toolchain"

target="aarch64-phoenix"

if [ -x "${toolchain_dir}/${target}/bin/${target}-gcc" ]; then
	printf '[build-toolchain] already installed: %s/bin/%s-gcc\n' \
		"${toolchain_dir}/${target}" "${target}"
	exit 0
fi

# build-toolchain.sh expects to be run from within phoenix-rtos-build/
# with neighbouring phoenix-rtos-kernel and libphoenix at ../. Our
# layout puts them all under ${sources_dir}/.
helper="${sources_dir}/phoenix-rtos-build/toolchain/build-toolchain.sh"
if [ ! -x "${helper}" ]; then
	chmod +x "${helper}" 2>/dev/null || true
fi
if [ ! -f "${helper}" ]; then
	printf 'error: missing %s\n' "${helper}" >&2
	exit 1
fi

# build-toolchain.sh refuses if CC/CFLAGS/etc. are set in the env.
unset CC CFLAGS LIBS CPPFLAGS CXX CXXFLAGS CPP CXXCPP CXXFILT 2>/dev/null || true

# Need bison + flex + gmp/mpfr/mpc dev headers + texinfo for the build.
need=(build-essential bison flex texinfo \
	libgmp-dev libmpfr-dev libmpc-dev \
	wget xz-utils)
missing=()
for p in "${need[@]}"; do
	dpkg -s "$p" >/dev/null 2>&1 || missing+=("$p")
done
if [ "${#missing[@]}" -gt 0 ]; then
	printf '[build-toolchain] installing build prerequisites: %s\n' "${missing[*]}"
	sudo apt-get install -y --no-install-recommends "${missing[@]}"
fi

mkdir -p "${toolchain_dir}"

printf '[build-toolchain] starting build (this takes 20-60 min on a fast box)...\n'
printf '[build-toolchain] target=%s install=%s\n' "${target}" "${toolchain_dir}"

# Run the helper. It expects to find phoenix-rtos-kernel and libphoenix
# at ../ relative to itself, which matches our layout.
"${helper}" "${target}" "${toolchain_dir}"

if [ -x "${toolchain_dir}/${target}/bin/${target}-gcc" ]; then
	printf '[build-toolchain] DONE. Add this to your PATH:\n'
	printf '    export PATH=%s/%s/bin:$PATH\n' "${toolchain_dir}" "${target}"
else
	printf '[build-toolchain] FAILED — %s-gcc not produced\n' "${target}" >&2
	exit 1
fi
