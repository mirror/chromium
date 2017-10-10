#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple chain containing only a leaf certificate."""

import sys
sys.path += ['..']

import common

# Self-signed root certificate.
root = common.create_self_signed_root_certificate('Root')

# Target certificate.
target = common.create_end_entity_certificate('Target', root)

chain = [target]
common.write_chain(__doc__, chain, 'chain.pem')

