#!/bin/bash

set -o pipefail
set -o nounset
set -e

if [[ -z "$1" ]]; then
    echo "Missing source directory argument." >&2
    exit 1
fi

# Deploying the environment requires 'CAP_SYS_ADMIN'. We do it at runtime at the first start.

if [[ ! -d "/toolkit/build_env/ds.${PLATFORM}-${TOOLKIT_VER}" ]]; then

    if ! capsh --print | grep -q 'cap_sys_admin.*ep'; then
        echo "Missing CAP_SYS_ADMIN privilege." >&2
        echo "Please make sure to run the container as root and use: '--cap-add=CAP_SYS_ADMIN'." >&2
        exit 1
    fi

    ./EnvDeploy -v "${TOOLKIT_VER}" -p "${PLATFORM}" -D
fi

# The Synology Toolkit attempts to hardlink the package source files to the chroot.
# We have to copy the project files to the local filesystem to avoid invalid cross-device link 
# errors.

rm -rf /toolkit/source/*
cp -a /toolkit/sources/* /toolkit/source

# Build the package.

./PkgCreate.py -v "${TOOLKIT_VER}" -p "${PLATFORM}" -c "$1"

# Move build artifacts to output directory.

mv /toolkit/build_env/ds."${PLATFORM}"-"${TOOLKIT_VER}"/image/packages/*.spk /toolkit/packages
