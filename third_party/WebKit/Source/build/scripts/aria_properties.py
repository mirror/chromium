#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import template_expander
import argparse
import os
import os.path
import re
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..',
                             'third_party', 'pyjson5'))
import json5

class ARIAWriter(object):
    def __init__(self, json5_file_path):
        self.json5_file_path = json5_file_path

    def load_json(self)
        with open(os.path.abspath(json5_file_path)) as json5_file:
            data = json5.loads(json5_file.read())
            return data


def main(self):
    parser = argparse.ArgumentParser()
    # Require exactly one input file.
    parser.add_argument("files", nargs="1")

    # Passed in automatically by css_properties template
    # parser.add_argument("--gperf", default="gperf")
    parser.add_argument("--output_dir", default=os.getcwd())
    args = parser.parse_args()

    aria_writer = ARIAWriter(args.files)
    aria_writer.load_json()

if __name__ == '__main__':
    main()
