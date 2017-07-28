#!/bin/bash

set -e

if [ $# -ne 1 ] ; then
  echo "This tool will download the full CertificateTransparency database and write it to a file (or synchronize an existing one)"
  echo ""
  echo "Usage: <path-to-output-ct-db>"
  exit 1
fi

# Make sure GO is installed
go version >/dev/null 2>&1 || { echo >&2 "ERROR: Need to have GO installed"; exit 1; }

OUT_PATH="$(readlink -f "$1")"

MY_DIR=$(mktemp -d)

echo "Created temporary directory: $MY_DIR"

cd "$MY_DIR"
export GOPATH="$MY_DIR"

echo "Downloading agl's certificatetransparency code into $MY_DIR"
go get github.com/agl/certificatetransparency

echo "Executing src/github.com/agl/certificatetransparency/tools/ct-sync.go"
go run src/github.com/agl/certificatetransparency/tools/ct-sync.go "$OUT_PATH"

# Cleanup this temporary directory.
echo "Cleaning up and deleting $MY_DIR"
rm -rf "$MY_DIR"
