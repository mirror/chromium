# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.layout_tests.try_flag import TryFlag


class TryFlagTest(unittest.TestCase):

    def test_main(self):
        TryFlag(['update'], MockHost()).run()
