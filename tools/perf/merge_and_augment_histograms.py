#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import tempfile

from tracing.value import merge_histograms

def main():
  MergeHistograms()
  AddReservedDiagnostics()  # Think about how to factor this with add_reserved_diagnostics

def MergeHistograms(histogram_json_path):
  # Merge histograms across story repeats
  per_story_histograms = merge_histograms.MergeHistograms(
      histogram_json_path, groupby=['name', 'story'])
  # Merge histograms across stories and story repeats
  summary_histograms = merge_histograms.MergeHistograms(
      histogram_json_path, groupby=['name'])
  new_histograms = histogram_set.HistogramSet()
  new_histograms.ImportDicts(per_story_histograms)
  new_histograms.ImportDicts(summary_histograms)
  return json.dumps(new_histograms.AsDicts())
