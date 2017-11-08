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

#include "tools/cygprofile/anchor_functions.h"
#include "base/logging.h"

extern "C" {
void dummy_function_to_check_ordering() {}
void dummy_function_to_anchor_text() {}
void dummy_function_at_the_end_of_text() {}
}

namespace cygprofile {

const size_t kStartOfText =
    reinterpret_cast<size_t>(dummy_function_to_anchor_text);
const size_t kEndOfText =
    reinterpret_cast<size_t>(dummy_function_at_the_end_of_text);

void CheckOrderingSanity() {
  // The linker usually keeps the input file ordering for symbols.
  // dummy_function_to_anchor_text() should then be after
  // dummy_function_to_check_ordering() without ordering.
  // This check is thus intended to catch the lack of ordering.
  CHECK_LT(kStartOfText,
           reinterpret_cast<size_t>(&dummy_function_to_check_ordering));
  CHECK_LT(kStartOfText, kEndOfText);
  CHECK_LT(kStartOfText,
           reinterpret_cast<size_t>(&dummy_function_to_check_ordering));
  CHECK_LT(kStartOfText, reinterpret_cast<size_t>(&CheckOrderingSanity));
  CHECK_GT(kEndOfText, reinterpret_cast<size_t>(&CheckOrderingSanity));
}

}  // namespace cygprofile
