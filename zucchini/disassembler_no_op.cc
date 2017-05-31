// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/disassembler_no_op.h"

namespace zucchini {

DisassemblerNoOp::DisassemblerNoOp(Region image) : Disassembler(image) {}

DisassemblerNoOp::~DisassemblerNoOp() = default;

ExecutableType DisassemblerNoOp::GetExeType() const {
  return EXE_TYPE_NO_OP;
}

std::string DisassemblerNoOp::GetExeTypeString() const {
  return "(Unknown)";
}

ReferenceTraits DisassemblerNoOp::GetReferenceTraits(uint8_t type) const {
  return {};
}

uint8_t DisassemblerNoOp::GetReferenceTraitsCount() const {
  return 0;
}

bool DisassemblerNoOp::Parse() {
  return true;
}

}  // namespace zucchini
