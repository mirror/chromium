#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Usage: prefix_list.py prefix l1 l2 l3
Prints args 2.. prefixed with |prefix|."""

import sys
print '\n'.join([sys.argv[1] + arg for arg in sys.argv[2:]])
