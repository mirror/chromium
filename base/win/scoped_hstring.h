// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_SCOPED_HSTRING_H_
#define BASE_WIN_SCOPED_HSTRING_H_

#include <hstring.h>
#include <winstring.h>

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
  BASE_EXPORT static void Free(HSTRING hstr) { WindowsDeleteString(hstr); }
};

BASE_EXPORT class ScopedHString
    : public base::ScopedGeneric<HSTRING, ScopedHStringTraits> {
 public:
  BASE_EXPORT static ScopedHString Create(const base::char16* str);
  BASE_EXPORT static ScopedHString Create(HSTRING hstr);

  // Loads all required HSTRING functions. These tend to be consistent since
  // Win8.
  BASE_EXPORT static bool PreloadRequiredFunctions();

  BASE_EXPORT std::string GetString();
  BASE_EXPORT base::StringPiece16 GetString16();

 private:
  explicit ScopedHString(HSTRING hstr);
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_SCOPED_HSTRING_H_
