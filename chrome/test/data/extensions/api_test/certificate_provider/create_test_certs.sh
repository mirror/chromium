#!/bin/bash

# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generates the following tree of certificates:
#     root1 (self-signed root)
#      \
#       \--> root1_l1_leaf (end-entity)
#     root2 (self-signed root)
#      \
#       \--> root2_l1_leaf (end-entity)
# Additionally, generates private key files (for signing at runtime) and
# DER-encoded issuer DistinguishedName files (for cert selection).

try() {
  "$@" || {
    e=$?
    echo "*** ERROR $e ***  $@  " > /dev/stderr
    exit $e
  }
}

# Create a self-signed CA cert with CommonName CN and store it at out/$1.pem .
root_cert() {
  try /bin/sh -c "echo 01 > out/${1}-serial"
  try touch out/${1}-index.txt
  try openssl genrsa -out out/${1}.key 2048

  CA_ID=$1 \
    try openssl req \
      -new \
      -key out/${1}.key \
      -out out/${1}.req \
      -config ca.cnf

  CA_ID=$1 \
    try openssl x509 \
      -req -days 3650 \
      -in out/${1}.req \
      -signkey out/${1}.key \
      -extfile ca.cnf \
      -extensions ca_cert > out/${1}.pem
}

# Create a cert with CommonName CN signed by CA_ID and store it at out/$1.der .
# $2 must either be "leaf_cert" (for a server/user cert) or "ca_cert" (for a
# intermediate CA).
# Stores the private key at out/$1.pk8 .
issue_cert() {
  if [[ "$2" == "ca_cert" ]]
  then
    try /bin/sh -c "echo 01 > out/${1}-serial"
    try touch out/${1}-index.txt
  fi
  try openssl req \
    -new \
    -keyout out/${1}.key \
    -out out/${1}.req \
    -config ca.cnf

  try openssl ca \
    -batch \
    -extensions $2 \
    -in out/${1}.req \
    -out out/${1}.pem \
    -config ca.cnf

  try openssl pkcs8 -topk8 -in out/${1}.key -out out/${1}.pk8 -outform DER \
    -nocrypt

  try openssl x509 -in out/${1}.pem -outform DER -out out/${1}.der
}

try rm -rf out
try mkdir out

# Generate:
# - out/root1.pem
# - out/root1_l1_leaf.der
# - out/root1_l1_leaf.key
# - out/root1_l1_leaf.pk8
CN=root1 \
  try root_cert root1

CA_ID=root1 CN=root1_l1_leaf \
  try issue_cert root1_l1_leaf leaf_cert

# Generate:
# - out/root2.pem
# - out/root2_l1_leaf.der
# - out/root2_l1_leaf.key
# - out/root2_l1_leaf.pk8
CN=root2 \
  try root_cert root2

CA_ID=root2 CN=root2_l1_leaf \
  try issue_cert root2_l1_leaf leaf_cert

# Generate:
# - out/root1_l1_leaf_issuer.der
#   contains the DER-encoded issuer of out/root1_l1_leaf.der.
# - out/root2_l1_leaf_issuer.der
#   contains the DER-encoded issuer of out/root2_l1_leaf.der.
try openssl asn1parse -in out/root1_l1_leaf.der -inform DER -strparse 31 -out \
  out/root1_l1_leaf_issuer_dn.der

try openssl asn1parse -in out/root2_l1_leaf.der -inform DER -strparse 31 -out \
  out/root2_l1_leaf_issuer_dn.der

# Creates a file out/data_to_sign containing data which shall be signed in the
# test runs.
# Creates files data_signature_sha{1,256,384,512} which contain the signature of
# data_to_sign, signed by root2_l1_leaf.der's private key, with varying hashing
# algorithms.
try echo -n "hello world" > out/data_to_sign
try openssl dgst -sha1 -sign out/root2_l1_leaf.key -out \
   out/data_signature_sha1_pkcs out/data_to_sign
try openssl dgst -sha256 -sign out/root2_l1_leaf.key -out \
   out/data_signature_sha256_pkcs out/data_to_sign
try openssl dgst -sha384 -sign out/root2_l1_leaf.key -out \
   out/data_signature_sha384_pkcs out/data_to_sign
try openssl dgst -sha512 -sign out/root2_l1_leaf.key -out \
   out/data_signature_sha512_pkcs out/data_to_sign

# Now copy the required files to the extension directories:
# certificateProvider test_extension needs only the .der files to provide the
# certificates.
test_extension_dir=test_extension
cp out/root1_l1_leaf.der ${test_extension_dir}/
cp out/root2_l1_leaf.der ${test_extension_dir}/

# platform_keys_bridge_test_extension needs:
# issuer DERs to filter
platform_keys_bridge_test_extension_dir=platform_keys_bridge_test_extension
cp out/root1_l1_leaf_issuer_dn.der ${platform_keys_bridge_test_extension_dir}/
cp out/root2_l1_leaf_issuer_dn.der ${platform_keys_bridge_test_extension_dir}/
# cert DERs to check if selection worked fine
cp out/root1_l1_leaf.der ${platform_keys_bridge_test_extension_dir}/
cp out/root2_l1_leaf.der ${platform_keys_bridge_test_extension_dir}/
# data to be signed and expected signatures
cp out/data_to_sign ${platform_keys_bridge_test_extension_dir}/
cp out/data_signature_sha1_pkcs  ${platform_keys_bridge_test_extension_dir}/
cp out/data_signature_sha256_pkcs  ${platform_keys_bridge_test_extension_dir}/
cp out/data_signature_sha384_pkcs  ${platform_keys_bridge_test_extension_dir}/
cp out/data_signature_sha512_pkcs  ${platform_keys_bridge_test_extension_dir}/

# the C++ side test code needs:
# The private keys to sign data
cp out/root1_l1_leaf.pk8 ./
cp out/root2_l1_leaf.pk8 ./
