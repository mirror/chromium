#!/bin/bash

set -e

if [ $# -ne 2 ] ; then
  echo "This tool will extract all of the certificates in a CertificateTransparency database and write their raw DER to a single file. This is the input file for cert_mapper"
  echo ""
  echo "Usage: <input-path-to-ct-db> <output-path-to-dump>"
  exit 1
fi

# Make sure GO is installed
go version >/dev/null 2>&1 || { echo >&2 "ERROR: Need to have GO installed"; exit 1; }

CT_DB_INPUT="$(readlink -f "$1")"
OUT_PATH="$(readlink -f "$2")"

ORIGINAL_GO_FILE=$( readlink -f "$(dirname "$0")/dump-ct.go" )

MY_DIR=$(mktemp -d)

echo "Created temporary directory: $MY_DIR"

cd "$MY_DIR"
export GOPATH="$MY_DIR"

echo "Downloading agl's certificatetransparency code into $MY_DIR"
go get github.com/agl/certificatetransparency

mkdir mycttools
DST_GO_FILE="mycttools/dump-ct.go"
echo "Copying to $ORIGINAL_GO_FILE to $MY_DIR/$DST_GO_FILE"
cp "$ORIGINAL_GO_FILE" "$MY_DIR/$DST_GO_FILE"

echo "Executing dump-ct.go"
go run "$DST_GO_FILE" "$CT_DB_INPUT" "$OUT_PATH"

# Cleanup this temporary directory.
echo "Cleaning up and deleting $MY_DIR"
rm -rf "$MY_DIR"
