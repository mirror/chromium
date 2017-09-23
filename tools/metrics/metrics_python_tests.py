#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import os
import sys
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__),
  '..', '..', 'testing'))
import isolated_test_utils

sys.path.append(os.path.join(os.path.dirname(__file__), 'actions'))
import extract_actions_test

sys.path.append(os.path.join(os.path.dirname(__file__), 'histograms'))
import generate_expired_histograms_array_unittest

sys.path.append(os.path.join(os.path.dirname(__file__), 'ukm'))
import pretty_print_test as ukm_pretty_print_test

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'variations'))
import fieldtrial_to_struct_unittest
import fieldtrial_util_unittest

sys.path.append(os.path.join(os.path.dirname(__file__),
  '..', '..', 'components', 'variations', 'service'))
import generate_ui_string_overrider_unittest


def main():
  modules = [
    extract_actions_test,
    generate_expired_histograms_array_unittest,
    # TODO(asvitkine): Include rappor/pretty_print_test.py here once it's fixed.
    ukm_pretty_print_test,
    fieldtrial_to_struct_unittest,
    fieldtrial_util_unittest,
    generate_ui_string_overrider_unittest]
  isolated_test_utils.ParseArgsAndRunTestInModules(modules)


if __name__ == "__main__":
  main()
