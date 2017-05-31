#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Creates a l1_leaf_issuer.der containing the DER-encoded issuer of
# ../l1_leaf.der.

try() {
  "$@" || {
    e=$?
    echo "*** ERROR $e ***  $@  " > /dev/stderr
    exit $e
  }
}

certificate_provider_dir=..

cp "${certificate_provider_dir}/l1_leaf.der" ./l1_leaf.der
# Using -strparse 31 offset here because this is a SHA-1 certificate.
try openssl asn1parse -in l1_leaf.der -inform DER -strparse 31 -out \
  l1_leaf_issuer_dn.der
try echo -n "hello world" > data_to_sign
try openssl pkcs8 -in "${certificate_provider_dir}/l1_leaf.pk8" -inform DER \
  -out ./tmp_l1_leaf.key -nocrypt
try openssl dgst -sha1 -sign tmp_l1_leaf.key -out \
   data_signature_sha1_pkcs data_to_sign
rm ./tmp_l1_leaf.key
