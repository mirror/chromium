# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import subprocess
import sys
import threading

CHROME_SRC = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir, os.pardir)
LLVM_SYMBOLIZER_PATH = os.path.join(
  CHROME_SRC, 'third_party', 'llvm-build', 'Release+Asserts', 'bin',
  'llvm-symbolizer')

BINARY = re.compile(r'0b[0,1]+')
HEX = re.compile(r'0x[0-9,a-e]+')
OCTAL = re.compile(r'0[0-7]+')

UNKNOWN = '<UNKNOWN>'

class LLVMSymbolizerInstance(object):
  def __init__(self):
    if os.path.isfile(LLVM_SYMBOLIZER_PATH):
      self._llvm_symbolizer_subprocess = subprocess.Popen([LLVM_SYMBOLIZER_PATH],
                                                         stdout=subprocess.PIPE,
                                                         stdin=subprocess.PIPE)
    else:
      logging.error('Cannot find llvm_symbolizer.')
      self._llvm_symbolizer_subprocess = None

    # Allow only one thread to call GetSymbolInformation at a time.
    self._lock = threading.Lock()
    self._closed_called = False

  def CheckValidAddr(self, addr):
    """
    Check whether the addr is valid input to llvm symbolizer.
    Valid addr has to be octal, binary, or hex number.

    Args:
      addr: addr to be entered to llvm symbolizer.

    Returns:
      whether the addr is valid input to llvm symbolizer.
    """
    return HEX.match(addr) or OCTAL.match(addr) or BINARY.match(addr)


  def GetSymbolInformation(self, lib, addr):
    """Return the corresponding function names and line numbers.

    Args:
      lib: library to search for info.
      addr: address to look for info.

    Returns:
      A list of (function name, line numbers) tuple.
    """
    if self._llvm_symbolizer_subprocess is None or not lib or not self.CheckValidAddr(addr) or not os.path.isfile(lib):
      return [(UNKNOWN, lib)]

    with self._lock:
      self._llvm_symbolizer_subprocess.stdin.write('%s %s\n' % (lib, addr))

      result = []
      # Read till see new line, which is a symbol of end of output.
      # One line of function name is always followed by one line of line number.
      while True:
        line = self._llvm_symbolizer_subprocess.stdout.readline()
        if line != '\n':
          result.append((line[:-1], self._llvm_symbolizer_subprocess.stdout.readline()[:-1]))
        else:
          break
      return result

  def Close(self):
    with self._lock:
      self._llvm_symbolizer_subprocess.stdin.close()
      self._llvm_symbolizer_subprocess.stdout.close()
      self._llvm_symbolizer_subprocess.kill()
      self._llvm_symbolizer_subprocess.wait()
      self._closed_called = True

  def __del__(self):
    if not self._closed_called:
      logging.error('Forgot to close llvm symbolizer.')
      self.Close()