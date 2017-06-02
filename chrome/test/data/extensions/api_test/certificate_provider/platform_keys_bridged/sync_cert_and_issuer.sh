#!/bin/bash
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Creates a root1_l1_leaf_issuer.der containing the DER-encoded issuer of
# ../root1_l1_leaf.der.
# Creates a root2_l1_leaf_issuer.der containing the DER-encoded issuer of
# ../root2_l1_leaf.der.
# Creates a file data_to_sign containing data which will be signed.
# Creates files data_signature_sha{1,256,384,512} which contain the signature of
# data_to_sign, signed by root2_l1_leaf.der's private key, with varying hashing
# algorithms.

try() {
  "$@" || {
    e=$?
    echo "*** ERROR $e ***  $@  " > /dev/stderr
    exit $e
  }
}

certificate_provider_dir=..

cp "${certificate_provider_dir}/root1_l1_leaf.der" ./root1_l1_leaf.der
try openssl asn1parse -in root1_l1_leaf.der -inform DER -strparse 31 -out \
  root1_l1_leaf_issuer_dn.der

cp "${certificate_provider_dir}/root2_l1_leaf.der" ./root2_l1_leaf.der
try openssl asn1parse -in root2_l1_leaf.der -inform DER -strparse 31 -out \
  root2_l1_leaf_issuer_dn.der
try echo -n "hello world" > data_to_sign
try openssl pkcs8 -in "${certificate_provider_dir}/root2_l1_leaf.pk8" \
  -inform DER -out ./tmp_root2_l1_leaf.key -nocrypt
try openssl dgst -sha1 -sign tmp_root2_l1_leaf.key -out \
   data_signature_sha1_pkcs data_to_sign
try openssl dgst -sha256 -sign tmp_root2_l1_leaf.key -out \
   data_signature_sha256_pkcs data_to_sign
try openssl dgst -sha384 -sign tmp_root2_l1_leaf.key -out \
   data_signature_sha384_pkcs data_to_sign
try openssl dgst -sha512 -sign tmp_root2_l1_leaf.key -out \
   data_signature_sha512_pkcs data_to_sign
rm ./tmp_root2_l1_leaf.key
