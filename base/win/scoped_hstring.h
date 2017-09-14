// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_SCOPED_HSTRING_H_
#define BASE_WIN_SCOPED_HSTRING_H_

#include <hstring.h>

#include "base/scoped_generic.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece_forward.h"

namespace base {
namespace win {

// Scoped HSTRING class to maintain lifetime of HSTRINGs allocated with
// WindowsCreateString().
BASE_EXPORT class ScopedHStringTraits {
 public:
  BASE_EXPORT static HSTRING InvalidValue() { return nullptr; }
  BASE_EXPORT static void Free(HSTRING hstr);
};

// ScopedHString is a wrapper around an HSTRING. Note that it requires certain
// functions that are only available on Windows 8 and later, and that these
// functions need to be delayloaded to avoid breaking Chrome on Windows 7.
//
// This requires callers to use ResolveCoreWinRTStringDelayload() and checking
// its return value, *before* using ScopedHString.
//
// One-time Initialization for ScopedHString:
//
//   bool success = ScopedHString::ResolveCoreWinRTStringDelayload();
//   if (!success) {
//     // ScopeHString can be used.
//   } else {
//     // Handle error.
//   }
//
// Example use:
//
//   ScopedHString string = ScopedHString::Create(L"abc");
//
//     Also:
//
//   HSTRING win_string;
//   HRESULT hr = WindowsCreateString(..., &win_string);
//   ScopedHString string(win_string);
//
BASE_EXPORT class ScopedHString
    : public base::ScopedGeneric<HSTRING, ScopedHStringTraits> {
 public:
  BASE_EXPORT explicit ScopedHString(HSTRING hstr);

  BASE_EXPORT static ScopedHString Create(const base::StringPiece16& str);

  // Loads all required HSTRING functions. These tend to be consistent since
  // Win8.
  BASE_EXPORT static bool ResolveCoreWinRTStringDelayload();

  BASE_EXPORT const base::StringPiece16 Get() const;
  BASE_EXPORT std::string GetAsUTF8() const;

 private:
  static bool load_succeeded;
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_SCOPED_HSTRING_H_
