#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import os.path
import re
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))
import template_expander

PYJSON5_DIR = os.path.join(os.path.dirname(__file__),
                           '..', '..', '..', '..', '..', '..', 'pyjson5')
sys.path.append(PYJSON5_DIR)

import json5

class ARIAWriter(object):
    def __init__(self, json5_file_path):
        print "json5_file_path: ", json5_file_path
        self.json5_file_path = json5_file_path

    def load_json(self):
        with open(os.path.abspath(self.json5_file_path)) as json5_file:
            self.data = json5.loads(json5_file.read())

    @template_expander.use_jinja('../../templates/make_qualified_names.h.tmpl', filters=filters)
    def generate_attributes(self):



def main():
    print "argv: ", str(sys.argv)
    parser = argparse.ArgumentParser()

    parser.add_argument("--output_dir", default=os.getcwd())
    parser.add_argument("--gperf", default="gperf")
    parser.add_argument("files", nargs="+")

    args = parser.parse_args()

    aria_writer = ARIAWriter(args.files[0])
    aria_writer.load_json()
    print "json: ", aria_writer.json

if __name__ == '__main__':
    main()
