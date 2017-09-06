# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
    LLDB Support for Chrome Types

    Add the following to your .lldbinit file to add Chrome Type summaries in LLDB and Xcode:

    command script import {Path to SRC Root}/tools/lldb/lldb_chrome.py

"""

import lldb

def __lldb_init_module(debugger, dict):
    debugger.HandleCommand('type summary add --expand -F lldb_chrome.String16_Summary base::string16')

def String16_Summary(valobj, dict):
    name = valobj.GetName()
    data = lldb.frame.EvaluateExpression("%s.data();" % name)
    length = lldb.frame.EvaluateExpression("%s.length();" % name).GetValueAsUnsigned(0)
    contents = u""
    error = lldb.SBError()
    for i in xrange(0, length):
        contents = contents + unichr(data.GetPointeeData(i, 1).GetUnsignedInt16(error, 0))
    return "{ length = %s, contents = '%s' }" % (length, contents)
