// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_HSTRING_REF_H_
#define BASE_WIN_HSTRING_REF_H_

#include <Hstring.h>

#include "base/macros.h"
#include "base/strings/string16.h"

namespace base {
namespace win {

// A class for creating HStringReferences.
BASE_EXPORT class HStringRef {
 public:
  BASE_EXPORT explicit HStringRef(const base::char16* str);

  BASE_EXPORT HSTRING Get();

 private:
  HSTRING hstr_;
  HSTRING_HEADER header_;

  DISALLOW_COPY_AND_ASSIGN(HStringRef);
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_HSTRING_REF_H_
