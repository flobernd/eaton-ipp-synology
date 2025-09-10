#!/bin/bash

set -o pipefail
set -o nounset
set -e

if [ "$EUID" -ne 0 ]; then
    echo "This script must be run as root."
    exit 1
fi

# Configure 'synopoweroff' wrapper to always run as 'root'.
# -rwsr-x--- 1 root eaton-ipp synopoweroff

dir=$(dirname "$0")

chown root "${dir}/../target/configs/actions/synopoweroff"
chmod 4750 "${dir}/../target/configs/actions/synopoweroff"
