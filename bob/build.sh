#!/usr/bin/env sh
cd "$(dirname "$0")"
echo "builddir = build/mac\nexecutable = bob" | cat - template.ninja > build.ninja
export NINJA_STATUS='[%f/%t] %es '
ninja || exit 1
rm build.ninja
