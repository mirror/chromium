// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/rel32_utils.h"

namespace zucchini {

/******** Rel32GeneratorX86 ********/

Rel32GeneratorX86::Rel32GeneratorX86(
    const AddressTranslator* trans,
    Region image,
    std::vector<offset_t>::const_iterator first,
    std::vector<offset_t>::const_iterator last,
    offset_t upper)
    : target_translator_(trans->GetRVAToOffsetTranslator()),
      location_translator_(trans->GetOffsetToRVATranslator()),
      image_(image),
      cur_(first),
      last_(last),
      upper_(upper) {}

Rel32GeneratorX86::Rel32GeneratorX86(const Rel32GeneratorX86&) = default;

Rel32GeneratorX86::~Rel32GeneratorX86() = default;

bool Rel32GeneratorX86::operator()(Reference* ref) {
  while (cur_ < last_ && *cur_ < upper_) {
    offset_t location = *(cur_++);
    rva_t loc_rva = location_translator_(location);
    rva_t target_rva =
        loc_rva + 4 + Region::at<uint32_t>(image_.begin(), location);
    offset_t target = target_translator_(target_rva);
    // In rare cases, the most significant bit of |target| is set. This
    // interferes with label marking. A quick fix is to reject these.
    if (IsMarked(target)) {
      static LimitedOutputStream los(std::cerr, 10);
      if (!los.full()) {
        los << "Warning: Skipping mark-alised x86 rel32 target: ";
        los << AsHex<8>(location) << " -> " << AsHex<8>(target) << ".";
        los << std::endl;
      }
      continue;
    }
    ref->location = location;
    ref->target = target;
    return true;
  }
  return false;
}

/******** Rel32ReceptorX86 ********/

Rel32ReceptorX86::Rel32ReceptorX86(AddressTranslator* trans, Region image)
    : target_translator_(trans->GetOffsetToRVATranslator()),
      location_translator_(trans->GetOffsetToRVATranslator()),
      image_(image) {}

Rel32ReceptorX86::Rel32ReceptorX86(const Rel32ReceptorX86&) = default;

Rel32ReceptorX86::~Rel32ReceptorX86() = default;

void Rel32ReceptorX86::operator()(const Reference& ref) {
  rva_t target_rva = target_translator_(ref.target);
  rva_t rel32_rva = location_translator_(ref.location);
  Region::at<uint32_t>(image_.begin(), ref.location) =
      target_rva - (rel32_rva + 4);
}

}  // namespace zucchini
