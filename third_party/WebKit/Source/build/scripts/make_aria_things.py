#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import argparse
import hasher
import json
import name_utilities
import os
import os.path
import re
import sys

sys.path.append(os.path.dirname(__file__))
import template_expander
from aria_properties import ARIAReader

def _symbol(entry):
    return entry['name'].replace('-', '_')

class ARIAWriter(object):
    filters = {
        'hash': hasher.hash,
        'symbol': _symbol,
        'to_macro_style': name_utilities.to_macro_style,
    }

    def __init__(self, json5_file_path):
        print "json5_file_path: ", json5_file_path
        self._aria_reader = ARIAReader(json5_file_path)

        with open(os.path.abspath(self.json5_file_path)) as json5_file:
            print "json5_file: ", json5_file
            self.data = json5.loads(json5_file.read())
            namespace = self._metadata('namespace')
            namespace_prefix = self._metadata('namespacePrefix') or self.namespace.lower()
            namespace_uri = self._metadata('namespaceURI')
            use_namespace_for_attrs = self.data['metadata']['attrsNullNamespace'] is None
            self._template_context = {
                'attrs': self.data['attributes'],
                'export': self._metadata('export'),
                'input_files': self._input_files,
                'namespace': namespace,
                'namespace_prefix': namespace_prefix,
                'namespace_uri': namespace_uri,
                'use_namespace_for_attrs': use_namespace_for_attrs,
            }

    def _metadata(self, name):
        return self.data['metadata'][name].strip('"')

    def _write_file_if_changed(self, output_dir, contents, file_name):
        print "_write_file_if_changed", output_dir, file_name
        path = os.path.join(output_dir, file_name)

        # The build system should ensure our output directory exists, but just
        # in case.
        directory = os.path.dirname(path)
        if not os.path.exists(directory):
            os.makedirs(directory)

        # Only write the file if the contents have changed. This allows ninja to
        # skip rebuilding targets which depend on the output.
        with open(path, "a+") as output_file:
            output_file.seek(0)
            if output_file.read() != contents:
                output_file.truncate(0)
                output_file.write(contents)

    def write_files(self, output_dir):
        self._write_file_if_changed(output_dir, self.generate_header(), 'aria_attribute_names.h')

    @template_expander.use_jinja('templates/make_qualified_names.h.tmpl', filters=filters)
    def generate_header(self):
        return self._template_context



def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--output_dir", default=os.getcwd())
    parser.add_argument("--gperf", default="gperf")
    parser.add_argument("files", nargs="+")

    args = parser.parse_args()

    aria_writer = ARIAWriter(args.files[0])
    print json.dumps(aria_writer.data, indent=2)
    aria_writer.write_files(args.output_dir)

if __name__ == '__main__':
    main()
