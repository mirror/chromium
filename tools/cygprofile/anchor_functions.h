// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These functions are here to, respectively:
// 1. Check that functions are ordered
// 2. Delimit the start of .text
// 3. Delimit the end of .text
//
// (2) and (3) require a suitably constructed orderfile, with these
// functions at the beginning and end. (1) doesn't need to be in it.
//
// Also note that we have "load-bearing" CHECK()s below: they have side-effects.
// Without any code taking the address of a function, Identical Code Folding
// would merge these functions with other empty ones, defeating their purpose.

#include <cstdint>

namespace cygprofile {

extern const size_t kStartOfText;
extern const size_t kEndOfText;

void CheckOrderingSanity();

}  // namespace cygprofile
