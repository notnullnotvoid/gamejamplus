#!/usr/bin/env sh
cd "$(dirname "$0")"
bob/build.sh || exit 1
bob/bob -o build.ninja -c "$1" || exit 1
export NINJA_STATUS='[%f/%t] %es '
ninja || exit 1
rm build.ninja
# ulimit -c unlimited
./game
echo game exited with $?
