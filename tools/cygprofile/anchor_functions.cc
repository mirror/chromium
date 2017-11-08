// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cygprofile/anchor_functions.h"

#include "base/logging.h"
#include "build/build_config.h"

// ARM only, due to assembly below.
#if !defined(ARCH_CPU_ARMEL)
#error "Only supported on ARM"
#endif

// These functions are here to, respectively:
// 1. Check that functions are ordered
// 2. Delimit the start of .text
// 3. Delimit the end of .text
//
// (2) and (3) require a suitably constructed orderfile, with these
// functions at the beginning and end. (1) doesn't need to be in it.
//
// These functions are weird: this is due to ICF (Identical Code Folding).
// The linker merges functions that have the same code, which would be the case
// if these functions were empty, or simple.
// Gold's flag --icf=safe will *not* alias functions when their address is used
// in code, but as of November 2017, we use the default setting that
// deduplicates function in this case as well.
//
// Thus these functions are made to be unique, hence inline .word in assembly.
// - Starts with the "ret" instruction. No sane compiler would put code in a
//   function after an unconditional return without any branch, as this is
//   clearly dead code.
// - 2 random 4-byte constants.
//
// Note that code |CheckOrderingSanity()| below will make sure that these
// functions are not aliased, in case we get clever LTO one day.
extern "C" {
void dummy_function_to_check_ordering() {
  asm("bx lr");
  asm(".word 0xe19c683d");  // chosen by fair dice roll. Guaranteed to be
                            // random.
  asm(".word 0xb3d2b56");
}

void dummy_function_to_anchor_text() {
  asm("bx lr");
  asm(".word 0xe1f8940b");
  asm(".word 0xd5190cda");
}

void dummy_function_at_the_end_of_text() {
  asm("bx lr");
  asm(".word 0x133b9613");
  asm(".word 0xdcd8c46a");
}

}  // extern "C"

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
