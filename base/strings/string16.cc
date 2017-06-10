// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string16.h"

#if defined(WCHAR_T_IS_UTF16) && !defined(_AIX)

#error This file should not be used on 2-byte wchar_t systems
// If this winds up being needed on 2-byte wchar_t systems, either the
// definitions below can be used, or the host system's wide character
// functions like wmemcmp can be wrapped.

#elif defined(WCHAR_T_IS_UTF32)

#include <ostream>

#include "base/strings/utf_string_conversions.h"

namespace base {

std::ostream& operator<<(std::ostream& out, const string16& str) {
  return out << UTF16ToUTF8(str);
}

void PrintTo(const string16& str, std::ostream* out) {
  *out << str;
}

}  // namespace base

template class std::basic_string<base::char16, base::string16_char_traits>;

#endif  // WCHAR_T_IS_UTF32
