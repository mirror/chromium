// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_DISASSEMBLER_NO_OP_H_
#define ZUCCHINI_DISASSEMBLER_NO_OP_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/macros.h"
#include "zucchini/disassembler.h"
#include "zucchini/image_utils.h"
#include "zucchini/ranges/algorithm.h"
#include "zucchini/ranges/functional.h"
#include "zucchini/region.h"

namespace zucchini {

// This disassembler works on any file and does not look for reference.
class DisassemblerNoOp : public Disassembler {
 public:
  explicit DisassemblerNoOp(Region image);
  ~DisassemblerNoOp() override;

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  ReferenceTraits GetReferenceTraits(uint8_t type) const override;
  uint8_t GetReferenceTraitsCount() const override;
  bool Parse() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerNoOp);
};

}  // namespace zucchini

#endif  // ZUCCHINI_DISASSEMBLER_NO_OP_H_
