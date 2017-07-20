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
                               offset_t lo,
                               offset_t hi,
                               const std::vector<offset_t>* locations,
                               const AddressTranslatorFactory& factory)
    : target_rva_to_offset_(factory.MakeRVAToOffsetTranslator()),
      location_offset_to_rva_(factory.MakeOffsetToRVATranslator()),
      image_(image),
      hi_(hi),
      last_(locations->end()) {
  DCHECK_LE(lo, image.size());
  DCHECK_LE(hi, image.size());
  current_ = std::lower_bound(locations->begin(), locations->end(), lo);
}

Rel32ReaderX86::~Rel32ReaderX86() = default;

base::Optional<Reference> Rel32ReaderX86::GetNext() {
  while (current_ < last_ && *current_ < hi_) {
    offset_t location = *(current_++);
    rva_t loc_rva = location_offset_to_rva_->Apply(location);
    rva_t target_rva = loc_rva + 4 + image_.Read<int32_t>(location);
    offset_t target = target_rva_to_offset_->Apply(target_rva);
    // In rare cases, the most significant bit of |target| is set. This
    // interferes with label marking. We expect these to already
    // be filtered out from |locations|.
    DCHECK(!IsMarked(target));
    return Reference{location, target};
  }
  return base::nullopt;
}

/******** Rel32ReceptorX86 ********/

Rel32WriterX86::Rel32WriterX86(MutableBufferView image,
                               const AddressTranslatorFactory& factory)
    : target_offset_to_rva_(factory.MakeOffsetToRVATranslator()),
      location_offset_to_rva_(factory.MakeOffsetToRVATranslator()),
      image_(image) {}

Rel32WriterX86::~Rel32WriterX86() = default;

void Rel32WriterX86::PutNext(Reference ref) {
  rva_t target_rva = target_offset_to_rva_->Apply(ref.target);
  rva_t rel32_rva = location_offset_to_rva_->Apply(ref.location);
  image_.Write<uint32_t>(ref.location, target_rva - (rel32_rva + 4));
}

}  // namespace zucchini
