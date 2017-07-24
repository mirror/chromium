#!/usr/bin/python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This python script generates a test certificate chain where the end-entity
certificate uses a 1024-bit (weak) RSA key.

Must be run from the current directory.
"""

import sys
sys.path += ['../../../../../net/data/verify_certificate_chain_unittest']

import common

JAN_2015 = '150101120000Z'
JAN_2018 = '180101120000Z'

# Self-signed root certificate.
root = common.create_self_signed_root_certificate('Root')
root.set_validity_range(JAN_2015, JAN_2018)

# Intermediate certificate.
intermediate = common.create_intermediate_certificate('Intermediate', root)
intermediate.set_validity_range(JAN_2015, JAN_2018)

# Leaf certificate (using RSA 1024-bit key).
leaf = common.create_end_entity_certificate('RSA 1024 Device Cert',
                                            intermediate)
leaf.get_extensions().set_property('extendedKeyUsage', 'clientAuth')
device_key_path = common.create_key_path(leaf.name)
leaf.set_key(common.get_or_generate_rsa_key(1024, device_key_path))
leaf.set_validity_range(JAN_2015, JAN_2018)

chain = [leaf, intermediate, root]
chain_description = """Cast certificate chain where device certificate uses a
weak key size (1024-bit RSA)"""

# Write the certificate chain.
common.write_chain(chain_description, chain, 'rsa1024_device_cert.pem')
