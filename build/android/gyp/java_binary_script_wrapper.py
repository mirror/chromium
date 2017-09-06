#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wraps java_binary scripts for convenient use.

Enables passing @FileArgs to java binary scripts (since these are anoyying to
deal with in Java). Args will be passed to script in the following order:
input, output, switches, extra inputs.
"""

import argparse
import os
import sys

from util import build_utils


def _OnStaleMd5(script, input_path, output_path, switches, args):
  os.execv(script, [script] + [input_path] + [output_path] + switches + args)


if __name__ == '__main__':
  argv = build_utils.ExpandFileArgs(sys.argv[1:])
  parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(parser)
  parser.add_argument('--script', required=True,
                      help='Path to the java binary wrapper script.')
  parser.add_argument('--input', required=True,
                      help='The main input, passed first to the binary script.')
  parser.add_argument('--output', required=True,
                      help='Main output, passed second to the binary script.')
  parser.add_argument('--extra-input', dest='extra_inputs',
                      action='append', default=[],
                      help='Extra inputs, passed last to the binary script.')
  known_args, unknown_args = parser.parse_known_args(argv)
  extra_inputs = []
  for a in known_args.extra_inputs:
    extra_inputs.extend(build_utils.ParseGnList(a))

  build_utils.CallAndWriteDepfileIfStale(
      lambda: _OnStaleMd5(known_args.script, known_args.input,
                          known_args.output, unknown_args, extra_inputs),
      known_args,
      input_paths=[known_args.script, known_args.input],
      input_strings=unknown_args,
      output_paths=[known_args.output],
      depfile_deps=extra_inputs)
