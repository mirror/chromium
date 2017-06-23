#!/bin/bash

# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generates the following tree of certificates:
#     root1 (self-signed root)
#      \
#       \--> root1_l1_leaf (end-entity)
#       |--> root1_l1_leaf_other (end-entity)
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

# Find the offset of n-th element in ASN1 structure.
# ASN1 defines a nested data structure (tree).
# This function examines at the ASN1 structure of the DER-encoded
# certificate $1.
# $2 can be empty to look at the file from the beginning, or can consist of
# "-strparse <offset>" parameters passed to asn1parse to only look at the
# section of the file starting at <offset>
# $3 is the 1-based index of the element we want to extract (only looking at
# top-level elements).
extract_asn1_offset() {
  # Get the top-level (depth 1 / d=1) structure of the ASN1 certificate
  local ASN1_TOP_LEVEL_STRUCTURE=$(openssl asn1parse -in ${1} -inform DER $2 \
                                   | grep "d=1")

  # Select the n-th line, while n=$2
  local ASN1_ELEMENT=$(echo "$ASN1_TOP_LEVEL_STRUCTURE" | awk "NR == ${3}")

  # Get the offset (the number between the colon)
  local ASN1_ELEMENT_OFFSET=$(echo "$ASN1_ELEMENT" | cut -d : -f 1)

  # Return the offset
  echo $ASN1_ELEMENT_OFFSET
}

# Extracts the DistinguishedName (dn) of the issuer of the DER-encoded
# certificate stored at out/$1.der.
# The issuer is stored in DER econding int out/$2.der.
extract_cert_issuer_dn() {
  # The offset of the issuer DN is not constant.
  # According to RFC5280 section 4.1., the issuer is the fourth element
  # (issuer) under the first element (tbsCertificate).

  local CERT_IN=out/${1}.der

  # Offset to the top-level tbsCertificate field (see RFC5280 section 4.1)
  local OFFSET_TBS_CERTIFICATE=$(extract_asn1_offset $CERT_IN "" 1)

  # Offset to the issuer field (relative to OFFSET_TBS_CERTIFICATE)
  local OFFSET_ISSUER=$(extract_asn1_offset $CERT_IN \
                        "-strparse $OFFSET_TBS_CERTIFICATE" 4)

  # Now parse the issuer (starting at $ASN1_ISSUER_OFFSET) into the output file
  try openssl asn1parse -in $CERT_IN -inform DER \
    -strparse $OFFSET_TBS_CERTIFICATE -strparse $OFFSET_ISSUER \
    -out out/${2}.der
}

# Uses out/$1.key and hash algorithm $2 to sign out/$3 and outputs the
# signature to out/$4.
# Parameters:
# $1: key filename, will be expanded to out/$1.key
# $2: hash algorithm, sha1/sha256/sha384/sha512
# $3: input file, will be expanded to out/$3
# $4: output file, will be expanded to out/$4
sign() {
  try openssl dgst -${2} -sign out/${1}.key -out out/${4} out/${3}
}

try rm -rf out
try mkdir out

# Generate:
# - out/root1.pem
# - out/root1_l1_leaf.der
# - out/root1_l1_leaf.key
# - out/root1_l1_leaf.pk8
# - out/root1_l1_leaf_other.der
# - out/root1_l1_leaf_other.key
# - out/root1_l1_leaf_other.pk8
CN=root1 \
  try root_cert root1

CA_ID=root1 CN=root1_l1_leaf \
  try issue_cert root1_l1_leaf leaf_cert

CA_ID=root1 CN=root1_l1_leaf_other \
  try issue_cert root1_l1_leaf_other leaf_cert

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
try extract_cert_issuer_dn root1_l1_leaf root1_l1_leaf_issuer_dn
try extract_cert_issuer_dn root2_l1_leaf root2_l1_leaf_issuer_dn

# Creates a file out/data_to_sign containing data which shall be signed in the
# test runs.
try echo -n "hello world" > out/data_to_sign

# Creates files data_signature_sha{1,256,384,512} which contain the signature of
# data_to_sign, signed by root2_l1_leaf.der's private key, with varying hashing
# algorithms.
try sign root2_l1_leaf sha1 data_to_sign data_signature_sha1_pkcs
try sign root2_l1_leaf sha256 data_to_sign data_signature_sha256_pkcs
try sign root2_l1_leaf sha384 data_to_sign data_signature_sha384_pkcs
try sign root2_l1_leaf sha512 data_to_sign data_signature_sha512_pkcs

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
cp out/root1_l1_leaf_other.der ${platform_keys_bridge_test_extension_dir}/
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

# Also output the fingerprint hardcoded in apitest source code
echo
echo "ACTION REQUIRED:"
echo "Please update certificate_provider_apitest.cc hardcoded"
echo "root1_l1_leaf.der fingerprint to the following value:"
FINGERPRINT=$(openssl x509 -inform DER -noout -fingerprint -in \
  ${test_extension_dir}/root1_l1_leaf.der)
# Output is SHA1 Fingerprint=A0:B1:...
# We just want a0b1...
# So use awk to:
# - remove everything until the = separator
# - remove the colons
# - convert to lowercase
echo "$FINGERPRINT" \
  | awk '{ gsub(/.*=/, ""); gsub(":", ""); print tolower($0) }'
