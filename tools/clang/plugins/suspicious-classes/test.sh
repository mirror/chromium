#!/bin/bash -l

MY_PATH="$(dirname "$0")"
LLVM_BUILD_PATH="$MY_PATH/../../../../third_party/llvm-build/Release+Asserts"

echo "=== Build ==="

ninja -C "$LLVM_BUILD_PATH"

echo

echo "=== Run ==="

CLANG_PATH="$LLVM_BUILD_PATH/bin/clang++"
PLUGIN_PATH="$LLVM_BUILD_PATH/lib/libFindBadConstructs"

if [ "$(uname -s)" = "Darwin" ]; then
  PLUGIN_PATH="$PLUGIN_PATH.dylib"
else
  PLUGIN_PATH="$PLUGIN_PATH.so"
fi

output=$("$CLANG_PATH" \
    -ferror-limit=0 -std=c++11 -m32 \
    -Xclang -load -Xclang "$PLUGIN_PATH" \
    -Xclang -add-plugin -Xclang find-suspicious-classes \
    -c "$MY_PATH/test.cpp" 2>&1)

echo "$output"

echo
echo "Lines: $(echo "$output" | wc -l)"
