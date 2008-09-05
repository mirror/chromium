# Copyright 2008 Google Inc. All Rights Reserved.

# This module provides various utilities related to the platform. It is
# sometimes the case that certain built-in functions will only work on certain
# operating systems, requiring an alternative approach on unsupported
# platforms. This module provides access to these operations in a platform
# independent way.

import sys, os, signal

def _IsWindows():
    return sys.platform[:3] == 'win'

def Kill(pid):
    if _IsWindows():
        os.popen('taskkill /T /F /PID %d' % pid)
    else:
        os.kill(pid, signal.SIGTERM)
