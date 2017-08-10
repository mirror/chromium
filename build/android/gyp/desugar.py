#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import shutil
import sys
import tempfile
import zipfile

from util import build_utils


_SRC_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                         '..', '..', '..'))
_DESUGAR_JAR_PATH = os.path.normpath(os.path.join(
    _SRC_ROOT, 'third_party', 'bazel', 'desugar', 'Desugar.jar'))

_DESUGAR_RUNTIME_PREFIX = 'com/google/devtools/build/android/desugar/runtime'


def _OnStaleMd5(input_jar, output_jar, classpath, bootclasspath_entry):
  with tempfile.NamedTemporaryFile(delete=False) as tmp_out:
    cmd = [
        'java',
        '-jar',
        _DESUGAR_JAR_PATH,
        '--input',
        input_jar,
        '--bootclasspath_entry',
        bootclasspath_entry,
        '--output',
        tmp_out.name,
    ]
    for path in classpath:
      cmd += ['--classpath_entry', path]
    build_utils.CheckOutput(cmd, print_stdout=False)
    with zipfile.ZipFile(tmp_out.name) as tmp_zip:
      all_infos = tmp_zip.infolist()
      non_desugar_infos = [
          x for x in all_infos
          if not x.filename.startswith(_DESUGAR_RUNTIME_PREFIX)]
      # No try-with-resource files, just move the jar.
      if len(non_desugar_infos) == len(all_infos):
        shutil.move(tmp_out.name, output_jar)
      else:
        # Strip try-with-resources files from being included in every single
        # .jar. Instead, they are included once via
        # //third_party/bazel/desugar:desugar_runtime_java.
        with zipfile.ZipFile(output_jar, 'w') as out_zip:
          for info in non_desugar_infos:
            out_zip.writestr(info, tmp_zip.read(info))
        os.unlink(tmp_out.name)


def main():
  args = build_utils.ExpandFileArgs(sys.argv[1:])
  parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(parser)
  parser.add_argument('--input-jar', required=True,
                      help='Jar input path to include .class files from.')
  parser.add_argument('--output-jar', required=True,
                      help='Jar output path.')
  parser.add_argument('--classpath', required=True,
                      help='Classpath.')
  parser.add_argument('--bootclasspath-entry', required=True,
                      help='Path to javac bootclasspath interface jar.')
  options = parser.parse_args(args)

  options.classpath = build_utils.ParseGnList(options.classpath)
  input_paths = options.classpath + [
      options.bootclasspath_entry,
      options.input_jar,
  ]
  output_paths = [options.output_jar]
  depfile_deps = options.classpath + [_DESUGAR_JAR_PATH]

  build_utils.CallAndWriteDepfileIfStale(
      lambda: _OnStaleMd5(options.input_jar, options.output_jar,
                          options.classpath, options.bootclasspath_entry),
      options,
      input_paths=input_paths,
      input_strings=[],
      output_paths=output_paths,
      depfile_deps=depfile_deps)


if __name__ == '__main__':
  sys.exit(main())
