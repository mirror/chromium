# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper around logging to set some custom logging functionality."""

import contextlib
import logging
import os
import time
import sys

from pylib.constants import host_paths

_COLORAMA_PATH = os.path.join(
    host_paths.DIR_SOURCE_ROOT, 'third_party', 'colorama', 'src')

with host_paths.SysPath(_COLORAMA_PATH, position=0):
  import colorama

getLogger = logging.getLogger

critical = logging.critical
debug = logging.debug
error = logging.error
exception = logging.exception
info = logging.info
warning = logging.warning

DEBUG = logging.DEBUG
ERROR = logging.ERROR
CRITICAL = logging.CRITICAL
TEST_FAIL = 51
TEST_PASS = 52
TEST_TIME = 53
WARNING = logging.WARNING

class CustomFormatter(logging.Formatter):
  """Custom log formatter."""
  # pylint does not see members added dynamically in the constructor.
  # pylint: disable=no-member
  COLOR_MAP = {
    DEBUG: colorama.Fore.CYAN + colorama.Style.DIM,
    WARNING: colorama.Fore.YELLOW,
    ERROR: colorama.Back.RED,
    CRITICAL: colorama.Back.RED,
    TEST_PASS: colorama.Fore.GREEN + colorama.Style.BRIGHT,
    TEST_FAIL: colorama.Fore.RED + colorama.Style.BRIGHT,
    TEST_TIME: colorama.Fore.MAGENTA + colorama.Style.BRIGHT,
  }

  # override
  def __init__(self, color, fmt='%(threadName)-4s  %(message)s'):
    logging.Formatter.__init__(self, fmt=fmt)
    self._creation_time = time.time()
    self._should_colorize = color

  # override
  def format(self, record):
    msg = logging.Formatter.format(self, record)
    if 'MainThread' in msg[:19]:
      msg = msg.replace('MainThread', 'Main', 1)
    timediff = time.time() - self._creation_time

    level_tag = record.levelname[0]
    if self._should_colorize:
      level_tag = '%s%-1s%s' % (
          self.COLOR_MAP.get(record.levelno, ''),
          level_tag, colorama.Style.RESET_ALL)

    return '%s %8.3fs %s' % (level_tag, timediff, msg)

logging.addLevelName(TEST_TIME, u'\u231A') # Unicode character for clock
def test_time(message, *args, **kwargs):
  logger = logging.getLogger()
  if logger.isEnabledFor(TEST_TIME):
    # pylint: disable=protected-access
    logger._log(TEST_TIME, message, args, **kwargs)

logging.addLevelName(TEST_FAIL, u'\u274C') # Unicode character for a cross.
def test_fail(message, *args, **kwargs):
  logger = logging.getLogger()
  if logger.isEnabledFor(TEST_FAIL):
    # pylint: disable=protected-access
    logger._log(TEST_FAIL, message, args, **kwargs)


logging.addLevelName(TEST_PASS, u'\u2714') # Unicode character for a checkmark
def test_pass(message, *args, **kwargs):
  logger = logging.getLogger()
  if logger.isEnabledFor(TEST_PASS):
    # pylint: disable=protected-access
    logger._log(TEST_PASS, message, args, **kwargs)


def _init_logging():
  logging.getLogger().handlers = []
  handler = logging.StreamHandler(stream=sys.stderr)
  logging.getLogger().addHandler(handler)
  handler.setFormatter(CustomFormatter(color=sys.stderr.isatty()))


_init_logging()

