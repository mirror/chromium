// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_SCOPED_HSTRING_H_
#define BASE_WIN_SCOPED_HSTRING_H_

#include <Hstring.h>
#include <Winstring.h>

#include "base/scoped_generic.h"
#include "base/strings/string16.h"

namespace base {
namespace win {

// Scoped HSTRING class to maintain lifetime of HSTRINGs allocated with
// WindowsCreateString().
BASE_EXPORT class ScopedHStringTraits {
 public:
  BASE_EXPORT static HSTRING InvalidValue();
  BASE_EXPORT static void Free(HSTRING hstr);
};

BASE_EXPORT class ScopedHString
    : public base::ScopedGeneric<HSTRING, ScopedHStringTraits> {
 public:
  BASE_EXPORT explicit ScopedHString(const base::char16* str);

  // Loads all required HSTRING functions. These tend to be consistent since
  // Win8.
  BASE_EXPORT static bool PreloadRequiredFunctions();

  BASE_EXPORT static HRESULT WindowsCreateString(const base::char16* src,
                                                 uint32_t len,
                                                 HSTRING* out_hstr);

  BASE_EXPORT static HRESULT WindowsDeleteString(HSTRING hstr);

  BASE_EXPORT static const base::char16* WindowsGetStringRawBuffer(
      HSTRING hstr,
      uint32_t* out_len);
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_SCOPED_HSTRING_H_
