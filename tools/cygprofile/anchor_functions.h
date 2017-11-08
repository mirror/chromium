// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>

namespace cygprofile {

// Start and end of .text, respectively.
extern const size_t kStartOfText;
extern const size_t kEndOfText;

// Basic CHECK()s ensuring that the symbols above are correctly set.
void CheckOrderingSanity();

}  // namespace cygprofile
