// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_STRING16_H_
#define BASE_STRINGS_STRING16_H_

// WHAT:
// A version of std::basic_string that provides 2-byte characters even when
// wchar_t is not implemented as a 2-byte type. You can access this class as
// string16. We also define char16, which string16 is based upon.
//
// WHY:
// On Windows, wchar_t is 2 bytes, and it can conveniently handle UTF-16/UCS-2
// data. Plenty of existing code operates on strings encoded as UTF-16.
//
// On many other platforms, sizeof(wchar_t) is 4 bytes by default. We can make
// it 2 bytes by using the GCC flag -fshort-wchar. But then std::wstring fails
// at run time, because it calls some functions (like wcslen) that come from
// the system's native C library -- which was built with a 4-byte wchar_t!
// It's wasteful to use 4-byte wchar_t strings to carry UTF-16 data, and it's
// entirely improper on those systems where the encoding of wchar_t is defined
// as UTF-32.
//
// Here, we define string16, which is similar to std::wstring but replaces all
// libc functions with custom, 2-byte-char compatible routines. It is capable
// of carrying UTF-16-encoded data.

#include <string>

#include "base/base_export.h"

namespace base {

typedef char16_t char16;
typedef std::u16string string16;
typedef std::char_traits<char16_t> string16_char_traits;

BASE_EXPORT std::ostream& operator<<(std::ostream& out, const string16& str);

// This is required by googletest to print a readable output on test failures.
BASE_EXPORT void PrintTo(const string16& str, std::ostream* out);

}  // namespace base

#endif  // BASE_STRINGS_STRING16_H_
