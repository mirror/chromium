# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime

import generate_expired_histograms_array
import unittest


class UnitTest(unittest.TestCase):

  def test_GetExpiredHistograms(self):
    histograms = {
        "FirstHistogram": {
            "expiry_date": "2000/10/01"
        },
        "SecondHistogram": {
            "expiry_date": "2002/10/01"
        },
        "ThirdHistogram": {
            "expiry_date": "2001/10/01"
        },
        "FourthHistogram": {},
        "FifthHistogram": {
            "obsolete": "Has expired.",
            "expiry_date": "2000/10/01"
        }
    }

    base_date = datetime.date(2001, 10, 1)

    expired_histograms_names = (
        generate_expired_histograms_array._GetExpiredHistograms(
            histograms, base_date))

    self.assertEqual(expired_histograms_names, ["FirstHistogram"])

  def test_GenerateHeaderFileContent(self):
    header_filename = "test/test.h"
    namespace = "some_namespace"
    hash_datatype = "uint64_t"
    histogram_map = {
        1: "FirstHistogram",
        2: "SecondHistogram",
    }

    content = generate_expired_histograms_array._GenerateHeaderFileContent(
        header_filename, namespace, hash_datatype, histogram_map)
    expected_include_guard = "#ifndef TEST_TEST_H_"
    expected_namespace = "namespace some_namespace"
    expected_array = """const uint64_t kExpiredHistogramsHashes[] = {
  1,  // FirstHistogram
  2,  // SecondHistogram
};"""
    expected_number = """const size_t kNumExpiredHistograms = 2;"""
    self.assertIn(expected_include_guard, content)
    self.assertIn(expected_array, content)
    self.assertIn(expected_number, content)
    self.assertIn(expected_namespace, content)


if __name__ == "__main__":
  unittest.main()
