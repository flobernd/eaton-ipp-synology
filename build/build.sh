#!/bin/bash

set -o pipefail
set -o nounset
set -e

# Any x86_64 platform works since the 'eaton-ipp-synology' package targets the whole 
# platform-family.
toolkit_ver="7.2"
platform="epyc7002"
package_name="eaton-ipp-synology"

image_name="flobernd/dsm-builder-$platform-$toolkit_ver:latest"
container_name="dsm-builder-eaton-ipp"

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

force_build=false
force_recreate=false
for arg in "$@"; do
    if [[ "$arg" == "--force-build" ]]; then
        force_build=true
        continue
    fi
    if [[ "$arg" == "--force-recreate" ]]; then
        force_recreate=true
        continue
    fi
done

if [[ "$force_build" == "true" || -z "$(docker images -q $image_name)" ]]; then
    docker build \
        --build-arg "TOOLKIT_VER=$toolkit_ver" \
        --build-arg "PLATFORM=$platform" \
        -t "$image_name" \
        "$script_dir"
fi

if $force_recreate; then
    docker container rm -f "$container_name"
fi

if docker ps -a --format '{{.Names}}' | grep -wq "$container_name"; then

    if [ "$(docker inspect -f '{{.State.Running}}' "$container_name")" = "true" ]; then
        docker stop "$container_name"
    fi

    docker start -ai "$container_name"
    exit
fi

source_dir=$(realpath "$script_dir/..")

docker run -it --name "$container_name" \
    -v "$source_dir:/toolkit/sources/$package_name" \
    -v "$script_dir/packages:/toolkit/packages" \
    --cap-add=CAP_SYS_ADMIN \
    "$image_name" \
    "$package_name"
