// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ELEMENT_DETECTION_H_
#define CHROME_INSTALLER_ZUCCHINI_ELEMENT_DETECTION_H_

#include <stddef.h>

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// Attempts to detect an executable located at start of |image|. If found,
// returns the corresponding disassembler. Otherwise returns nullptr.
std::unique_ptr<Disassembler> MakeDisassemblerWithoutFallback(
    ConstBufferView image);

// Similar to MakeDisassemblerWithoutFallback(), but on failure, returns an
// instance of DisassemblerNoOp instead of nullptr.
std::unique_ptr<Disassembler> MakeDisassembler(ConstBufferView image);

// Attempts to create a disassembler corresponding to |exe_type| and initialize
// it with |image|, On failure, returns nullptr.
std::unique_ptr<Disassembler> MakeDisassemblerOfType(ConstBufferView image,
                                                     ExecutableType exe_type);

// Returns a new instance of DisassemblerNoOp.
std::unique_ptr<Disassembler> MakeNoOpDisassembler(ConstBufferView image);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ELEMENT_DETECTION_H_
