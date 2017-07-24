// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_export.h"

namespace base {

class BASE_EXPORT ScopedMayBlock {
 public:
  ScopedMayBlock();
  ~ScopedMayBlock();
};

}  // namespace base
