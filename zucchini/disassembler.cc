// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/disassembler.h"

#include <algorithm>

namespace zucchini {

/******** ReferenceGroup ********/

ReferenceGenerator ReferenceGroup::Find() const {
  return ((*disasm_).*(traits_.find_))(0, disasm_->size());
}

ReferenceGenerator ReferenceGroup::Find(offset_t lower, offset_t upper) const {
  return ((*disasm_).*(traits_.find_))(lower, upper);
}

ReferenceReceptor ReferenceGroup::Receive() const {
  return ((*disasm_).*(traits_.receive_))();
}

/******** GroupView ********/

ReferenceGroup GroupView::Cursor::get() const {
  return {disasm_->GetReferenceTraits(idx_), disasm_};
}

ReferenceGroup GroupView::Cursor::at(std::ptrdiff_t n) const {
  return {disasm_->GetReferenceTraits(idx_ + n), disasm_};
}

/******** Disassembler ********/

RVAToOffsetTranslator Disassembler::GetRVAToOffsetTranslator() const {
  return [](rva_t rva) { return static_cast<offset_t>(rva); };
}

OffsetToRVATranslator Disassembler::GetOffsetToRVATranslator() const {
  return [](offset_t offset) { return static_cast<rva_t>(offset); };
}

uint8_t Disassembler::GetMaxPoolCount() const {
  uint8_t traits_count = GetReferenceTraitsCount();
  uint8_t ret = 0;
  for (uint8_t type = 0; type < traits_count; ++type) {
    ret =
        std::max(ret, static_cast<uint8_t>(GetReferenceTraits(type).pool + 1));
  }
  return ret;
}

}  // namespace zucchini
