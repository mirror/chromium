# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess

from devil.utils import reraiser_thread
from pylib import constants


class Deobfuscator(object):
  def __init__(self, mapping_path):
    self._reader_thread = None
    script_path = os.path.join(
        constants.GetOutDirectory(), 'bin', 'java_deobfuscate')
    cmd = [script_path, mapping_path]
    self._proc = subprocess.Popen(
        cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

  def TransformLines(self, lines):
    """Deobfuscates obfuscated names found in the given lines.

    Args:
      lines: A list of strings without trailing newlines.

    Returns:
      A list of strings without trailing newlines.
    """
    assert self._proc
    if not lines:
      return []

    # Allow only one thread to communicate with the subprocess at a time.
    if self._reader_thread:
      self._reader_thread.join()

    out_lines = [None] * len(lines)

    def deobfuscate_reader():
      for i in xrange(len(lines)):
        out_lines[i] = self._proc.stdout.readline()[:-1]

    # TODO(agrieve): Can probably speed this up by only sending lines through
    #     that might contain an obfuscated name.
    self._reader_thread = reraiser_thread.ReraiserThread(deobfuscate_reader)
    self._reader_thread.start()
    self._proc.stdin.write('\n'.join(lines))
    self._proc.stdin.write('\n')
    self._proc.stdin.flush()
    self._reader_thread.join()
    self._reader_thread = None

    return out_lines

  def Close(self):
    if self._proc:
      self._proc.stdin.close()
      self._proc = None
    if self._reader_thread:
      self._reader_thread.join()
      self._reader_thread = None

  def __del__(self):
    logging.warning('Forgot to Close() deobfuscator')
    self.Close()
