#!/bin/bash
# build the linux native images using a docker container
set -eu

# build the container
docker build -t sentry-native-linux -f Dockerfile .

# find the root c directory
pushd "$(dirname "$0")"/../.. || exit
ROOT=$(pwd)
popd || exit
echo found root at $ROOT

# mount the root directory to the container and run the build script
docker run -rm -v $ROOT:/root sentry-native-linux /bin/bash -c "cd /root/Source/ThirdParty && pwd && ls -l && bash ./build_linux.sh"
