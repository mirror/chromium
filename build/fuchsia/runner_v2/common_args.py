# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os


def AddCommonArgs(arg_parser):
  arg_parser.add_argument('--package',
                          type=os.path.realpath, required=True,
                          help='Path to the package to execute.')
  arg_parser.add_argument('--package-manifest',
                          type=os.path.realpath, required=True,
                          help='Path to the Fuchsia package manifest file.')
  arg_parser.add_argument('--output-directory',
                          type=os.path.realpath, required=True,
                          help=('Path to the directory in which build files are'
                                ' located (must include build type).'))
  arg_parser.add_argument('--target-cpu', required=True,
                          help='GN target_cpu setting for the build.')
  arg_parser.add_argument('--device', '-d', action='store_true', default=False,
                          help='Run on hardware device instead of QEMU.')
  arg_parser.add_argument('--host', help='The IP of the target device. ' +
                          'Optional.')
  arg_parser.add_argument('--port', '-p', type=int, default=22,
                          help='The port of the SSH service running on the ' +
                               'device. Optional.')
  arg_parser.add_argument('--ssh_config', '-F',
                      help='The path to the SSH configuration used for '
                           'connecting to the target device.')
  arg_parser.add_argument('--verbose', '-v', default=False, action='store_true',
                          help='Show more logging information.')


def EvaluateCommonArgs(args):
  logging.basicConfig(level=(logging.DEBUG if args.verbose else logging.INFO))
