#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A tool to kill any leftover test processes, executed by buildbot.

Only works on Windows."""

import os
import subprocess
import sys


def KillAll(process_names):
  """Tries to kill all copies of each process in the processes list."""
  for process_name in process_names:
    Kill(process_name)

def Kill(process_name):
  command = ['taskkill.exe', '/f', '/t', '/im']
  subprocess.call(command + [process_name])


# rdpclip.exe is part of Remote Desktop.  It has a bug that sometimes causes
# it to keep the clipboard open forever, denying other processes access to it.
# Killing BuildConsole.exe usually stops an IB build within a few seconds.
# Unfortunately, killing devenv.com or devenv.exe doesn't stop a VS build, so
# we don't bother pretending.
processes=[
    # Utilities we don't build, but which we use or otherwise can't
    # have hanging around.
    'BuildConsole.exe',
    'httpd.exe',
    'outlook.exe',
    'perl.exe',
    'purifye.exe',
    'purifyW.exe',
    'python_slave.exe',
    'rdpclip.exe',
    'svn.exe',

    # When VC crashes during compilation, this process which manages the .pdb
    # file generation sometime hangs.
    'mspdbsrv.exe',
    # The JIT debugger may start when devenv.exe crashes.
    'vsjitdebugger.exe',
    # This process is also crashing once in a while during compile.
    'midlc.exe',

    # Things built by/for Chromium.
    'base_unittests.exe',
    'chrome.exe',
    'crash_service.exe',
    'debug_message.exe',
    'flush_cache.exe',
    'image_diff.exe',
    'installer_unittests.exe',
    'interactive_ui_tests.exe',
    'ipc_tests.exe',
    'memory_test.exe',
    'net_unittests.exe',
    'page_cycler_tests.exe',
    'perf_tests.exe',
    'plugin_tests.exe',
    'printing_unittests.exe',
    'reliability_tests.exe',
    'selenium_tests.exe',
    'startup_tests.exe',
    'tab_switching_test.exe',
    'test_shell.exe',
    'test_shell_tests.exe',
    'tld_cleanup.exe',
    'ui_tests.exe',
    'unit_tests.exe',
    'v8_shell.exe',
    'v8_mksnapshot.exe',
    'v8_shell_sample.exe',
    'wow_helper.exe',
]

if '__main__' == __name__:
  sys.exit(KillAll(processes))
