# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
    LLDB Support for Chromium types in Xcode

    Add the following to your ~/.lldbinit:
    command script import {Path to SRC Root}/tools/lldb/lldb_chrome.py
"""

import lldb
import struct

def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand('type summary add -F ' +
        'lldb_chrome.basestring16_SummaryProvider base::string16')

def basestring16_SummaryProvider(valobj, internal_dict):
    s = valobj.GetValueForExpressionPath('.__r_.__first_.__s')
    l = valobj.GetValueForExpressionPath('.__r_.__first_.__l')
    size = s.GetChildMemberWithName('__size_').GetValueAsUnsigned(0)
    # This is highly dependent on libc++ being compiled with little endian.
    is_short_string = size & 1 == 0
    if is_short_string:
        data = s.GetChildMemberWithName('__data_')
    else:
        data = l.GetChildMemberWithName('__data_')
    uint16s = []
    i = 0
    while True:
        error = lldb.SBError()
        uint16 = data.GetPointeeData(i, 1).GetUnsignedInt16(error, 0)
        if not uint16 or error.fail:
            break
        uint16s.append(uint16)
        i += 1
    fmt = '<%sH' % i
    byte_string = struct.pack(fmt, *uint16s)
    return '"' + byte_string.decode('utf-16').encode('utf-8') + '"'
