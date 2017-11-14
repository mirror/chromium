# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate C/C++ source from certificate.

Usage:
  generate-spdy-session-fuzzer-includes.py input_filename output_filename

Load the PEM block from `input_filename` certificate, perform base64 decoding
and hex encodeing, and save it in `output_filename` so that it can be directly
included from C/C++ source as a char array.
"""

import sys
import base64
import textwrap

"""Load PEM block from file |input_filename|."""
def LoadPemBlock(input_filename):
    with open(input_filename, 'r') as f:
        while True:
            line = next(f)
            if line == "-----BEGIN CERTIFICATE-----\n":
                break
        while True:
            line = next(f)
            if line == "-----END CERTIFICATE-----\n":
                return
            yield line[:-1]

input_filename = sys.argv[1]
output_filename = sys.argv[2]

# Load PEM block.
encoded_pem_block = "".join(LoadPemBlock(input_filename))

# Perform Base64 decoding.
decoded_pem_block = base64.b64decode(encoded_pem_block)

# Hex format, wrap at 80 columns, then write into |output_filename|.
with open(output_filename, 'w') as f:
    f.write(textwrap.fill(
        ", ".join("0x{:02x}".format(ord(c)) for c in decoded_pem_block), 80))
    f.write("\n")
