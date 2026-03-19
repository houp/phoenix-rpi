#!/usr/bin/env bash
#
# Prepare a disposable phoenix-rtos-project buildroot backed by sibling repos.
#

set -euo pipefail

usage() {
	cat <<EOF
Usage: $(basename "$0") [buildroot-dir]

Prepare a disposable local Phoenix buildroot that:
- copies the phoenix-rtos-project source tree
- keeps build artifacts out of the upstream working copy
- links the project component paths to sibling repos under sources/

Default buildroot:
  writable repo checkout: <repo>/buildroots/phoenix-rtos-project
  read-only shared checkout: ~/phoenix-buildroots/phoenix-rtos-project
EOF
}

die() {
	printf "error: %s\n" "$*" >&2
	exit 1
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
	usage
	exit 0
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
sources_dir="${repo_root}/sources"
project_src="${sources_dir}/phoenix-rtos-project"

if [ "$#" -ge 1 ]; then
	buildroot="$1"
elif [ -w "${repo_root}" ]; then
	buildroot="${repo_root}/buildroots/phoenix-rtos-project"
else
	buildroot="${HOME}/phoenix-buildroots/phoenix-rtos-project"
fi

[ -d "${sources_dir}" ] || die "missing sources directory: ${sources_dir}"
[ -d "${project_src}" ] || die "missing phoenix-rtos-project clone: ${project_src}"
[ -f "${project_src}/.gitmodules" ] || die "missing .gitmodules in ${project_src}"

mapfile -t submodule_paths < <(
	git -C "${project_src}" config -f .gitmodules --get-regexp '^submodule\..*\.path$' | awk '{print $2}' | sort
)

[ "${#submodule_paths[@]}" -gt 0 ] || die "no submodule paths found in ${project_src}/.gitmodules"

for path in "${submodule_paths[@]}"; do
	[ -d "${sources_dir}/${path}" ] || die "missing sibling repository for ${path}: ${sources_dir}/${path}"
done

mkdir -p "${buildroot}"

rsync_args=(
	-a
	--delete
	--exclude
	.git
	--exclude
	_build
	--exclude
	_boot
)

for path in "${submodule_paths[@]}"; do
	rsync_args+=(--exclude "${path}")
done

rsync "${rsync_args[@]}" "${project_src}/" "${buildroot}/"

for path in "${submodule_paths[@]}"; do
	dst="${buildroot}/${path}"
	rm -rf "${dst}"
	ln -sfn "${sources_dir}/${path}" "${dst}"
done

mkdir -p "${buildroot}/_build" "${buildroot}/_boot"

printf "Prepared buildroot: %s\n" "${buildroot}"
printf "Project source: %s\n" "${project_src}"
printf "Linked component paths:\n"
for path in "${submodule_paths[@]}"; do
	printf "  %s -> %s\n" "${path}" "$(readlink "${buildroot}/${path}")"
done
