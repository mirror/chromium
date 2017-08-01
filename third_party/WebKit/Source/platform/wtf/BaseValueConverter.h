// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WTF_BaseValueConverter_h
#define WTF_BaseValueConverter_h

#include "base/values.h"

namespace WTF {

template <typename T>
class BaseValueConverter {
 public:
  static base::Value Convert(T&& value) {
    return base::Value(std::forward<T>(value));
  }
};

}  // namespace WTF

#endif  // WTF_BaseValueConverter_h
