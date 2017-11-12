#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helpers for isolated script test wrappers."""

def load_filter_list(filter_file):
  lines = []
  with open(filter_file, 'r') as f:
    for line in f:
      # Eliminate completely empty lines for robustness.
      stripped = line.strip()
      if stripped:
        lines.append(stripped)
  return lines
