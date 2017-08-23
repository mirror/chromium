# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess

from devil.utils import reraiser_thread
from pylib import constants


_MINIUMUM_TIMEOUT = 5.0  # Large enough to account for process start-up.
_PER_LINE_TIMEOUT = .002  # Should be able to process 500 lines per second.


class Deobfuscator(object):
  def __init__(self, mapping_path):
    self._reader_thread = None
    script_path = os.path.join(
        constants.GetOutDirectory(), 'bin', 'java_deobfuscate')
    cmd = [script_path, mapping_path]
    self._proc = subprocess.Popen(
        cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    self._logged_error = False

  def TransformLines(self, lines):
    """Deobfuscates obfuscated names found in the given lines.

    If anything goes wrong (process crashes, timeout, etc), returns |lines|.

    Args:
      lines: A list of strings without trailing newlines.

    Returns:
      A list of strings without trailing newlines.
    """
    if not lines:
      return []

    # Allow only one thread to communicate with the subprocess at a time.
    if self._reader_thread:
      logging.warning('Having to wait for Java deobfuscation.')
      self._reader_thread.join()

    if self._proc.returncode is not None:
      if not self._logged_error:
        logging.warning('java_deobfuscate process exited with code=%d.',
                        self._proc.returncode)
        self._logged_error = True
      return lines

    out_lines = list(lines)

    def deobfuscate_reader():
      for i in xrange(len(lines)):
        out_lines[i] = self._proc.stdout.readline()[:-1]

    # TODO(agrieve): Can probably speed this up by only sending lines through
    #     that might contain an obfuscated name.
    self._reader_thread = reraiser_thread.ReraiserThread(deobfuscate_reader)
    self._reader_thread.start()
    try:
      self._proc.stdin.write('\n'.join(lines))
      self._proc.stdin.write('\n')
      self._proc.stdin.flush()
      timeout = max(_MINIUMUM_TIMEOUT, len(lines) * _PER_LINE_TIMEOUT)
      self._reader_thread.join(timeout)
      if self._reader_thread.is_alive():
        logging.error('java_deobfuscate timed out.')
        self.Close()
      self._reader_thread = None
    except IOError:
      logging.exception('Exception during java_deobfuscate')
      self.Close()

    return out_lines

  def Close(self):
    if self._proc.returncode is not None:
      self._proc.stdin.close()
      self._proc.kill()
    self._reader_thread = None

  def __del__(self):
    if self._proc.returncode is not None:
      logging.error('Forgot to Close() deobfuscator')
