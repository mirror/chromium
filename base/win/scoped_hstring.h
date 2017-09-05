// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_SCOPED_HSTRING_H_
#define BASE_WIN_SCOPED_HSTRING_H_

#include <Hstring.h>

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
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_SCOPED_HSTRING_H_
