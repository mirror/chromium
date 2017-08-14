// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/element_detection.h"

#include <utility>

#include "base/logging.h"
#include "chrome/installer/zucchini/disassembler_no_op.h"

namespace zucchini {

/******** Utility Functions ********/

std::unique_ptr<Disassembler> MakeDisassemblerWithoutFallback(
    ConstBufferView image) {
  // TODO(etiennep): Add disassembler implementations.
  return nullptr;
}

std::unique_ptr<Disassembler> MakeDisassembler(ConstBufferView image) {
  std::unique_ptr<Disassembler> disasm = MakeDisassemblerWithoutFallback(image);
  if (!disasm) {
    disasm = DisassemblerNoOp::Make(image);
    DCHECK(disasm);
  }
  return disasm;
}

std::unique_ptr<Disassembler> MakeDisassemblerOfType(ConstBufferView image,
                                                     ExecutableType exe_type) {
  switch (exe_type) {
    case kExeTypeNoOp:
      return DisassemblerNoOp::Make(image);
    default:
      return nullptr;
  }
}

std::unique_ptr<Disassembler> MakeNoOpDisassembler(ConstBufferView image) {
  return DisassemblerNoOp::Make(image);
}

}  // namespace zucchini
