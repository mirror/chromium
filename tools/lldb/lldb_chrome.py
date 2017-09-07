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
    debugger.HandleCommand('type summary add -F lldb_chrome.basestring16_SummaryProvider base::string16')

def basestring16_SummaryProvider(valobj, dict):
    s = valobj.GetValueForExpressionPath(".__r_.__first_.__s")
    l = valobj.GetValueForExpressionPath(".__r_.__first_.__l")
    size = s.GetChildMemberWithName("__size_").GetValueAsUnsigned(0)
    # This is highly dependent on libc++ being compiled with little endian.
    is_short_string = size & 1 == 0
    if is_short_string:
        data = s.GetChildMemberWithName("__data_")
    else:
        data = l.GetChildMemberWithName("__data_")
    contents = u""
    i = 0
    while True:
        char = data.GetPointeeData(i, 1).GetUnsignedInt16(lldb.SBError(), 0)
        if not char:
            break
        contents = contents + unichr(char)
        i = i + 1
    return '"' + contents.encode('utf-8') + '"'
