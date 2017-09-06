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
    debugger.HandleCommand('type summary add --python-function lldb_chrome.String16_Summary base::string16')

def String16_Summary(valobj, dict):
    short_data = valobj.GetValueForExpressionPath(".__r_.__first_.__s.__data_")
    short_contents = read_contents(short_data)
    long_data = valobj.GetValueForExpressionPath(".__r_.__first_.__l.__data_")
    long_contents = read_contents(long_data)
    combined_contents = short_contents + long_contents
    return "\"%s\"" % combined_contents.encode('utf-8')

def read_contents(data):
    contents = u""
    i = 0
    while True:
        char = data.GetPointeeData(i, 1).GetUnsignedInt16(lldb.SBError(), 0)
        if not char:
            break
        contents = contents + unichr(char)
        i = i + 1
    return contents
