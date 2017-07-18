// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/rel32_utils.h"

#include <algorithm>

#include "base/logging.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

/******** Rel32ReaderX86 ********/

Rel32ReaderX86::Rel32ReaderX86(ConstBufferView image,
                               const AddressTranslator& translator,
                               const std::vector<offset_t>& locations,
                               offset_t lower_bound,
                               offset_t upper_bound)
    : target_translator_(translator.GetRVAToOffsetTranslator()),
      location_translator_(translator.GetOffsetToRVATranslator()),
      image_(image),
      current_(
          std::lower_bound(locations.begin(), locations.end(), lower_bound)),
      last_(locations.end()),
      upper_bound_(upper_bound) {}

Rel32ReaderX86::~Rel32ReaderX86() = default;

base::Optional<Reference> Rel32ReaderX86::GetNext() {
  while (current_ < last_ && *current_ < upper_bound_) {
    offset_t location = *(current_++);
    rva_t loc_rva = location_translator_->OffsetToRVA(location);
    rva_t target_rva = loc_rva + 4 + image_.Read<int32_t>(location);
    offset_t target = target_translator_->RVAToOffset(target_rva);
    // In rare cases, the most significant bit of |target| is set. This
    // interferes with label marking. A quick fix is to reject these.
    if (IsMarked(target)) {
      LOG(WARNING) << "Skipping mark-aliased x86 rel32 target: "
                   << AsHex<8>(location) << " -> " << AsHex<8>(target) << ".";
      continue;
    }
    return Reference{location, target};
  }
  return base::nullopt;
}

/******** Rel32ReceptorX86 ********/

Rel32WriterX86::Rel32WriterX86(MutableBufferView image,
                               const AddressTranslator& translator)
    : target_translator_(translator.GetOffsetToRVATranslator()),
      location_translator_(translator.GetOffsetToRVATranslator()),
      image_(image) {}

Rel32WriterX86::~Rel32WriterX86() = default;

void Rel32WriterX86::PutNext(Reference ref) {
  rva_t target_rva = target_translator_->OffsetToRVA(ref.target);
  rva_t rel32_rva = location_translator_->OffsetToRVA(ref.location);
  image_.Write<uint32_t>(ref.location, target_rva - (rel32_rva + 4));
}

}  // namespace zucchini
