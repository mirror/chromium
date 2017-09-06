#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import sys

def run(command, extra_options=''):
  command = command + ' ' + extra_options
  print command
  return os.system(command)


def GenerateMojoBindings(output_path, input_files):
  if not os.path.exists(output_path):
    os.makedirs(output_path)
  generator = 'mojo/public/tools/bindings/mojom_bindings_generator.py'
  input_file = 'components/cronet/interfaces/cronet.mojom'
  # Precompile bindings templates
  run(generator + ' precompile -o ', output_path)
  # Generate C interface
  run(generator + ' --use_bundled_pylibs generate ' + input_file +
      ' --bytecode_path ' + output_path + ' -g c,c++,java')
  # -o ' + output_path)


def main():
  parser = optparse.OptionParser()
  parser.add_option('--output-path',
        help='Output path for generated bindings')

  options, input_files = parser.parse_args()
  GenerateMojoBindings(options.output_path, input_files)


if __name__ == '__main__':
  sys.exit(main())
