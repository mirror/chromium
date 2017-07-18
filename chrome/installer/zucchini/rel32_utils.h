// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_REL32_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_REL32_UTILS_H_

#include <memory>
#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// Given a vector of reference locations, reads x86/x64 rel32 references, and
// finds their target.
class Rel32ReaderX86 : public ReferenceReader {
 public:
  // |image| wraps the raw bytes of a binary containing rel32 references.
  // |translator| is an suitable address translator for the binary |image|.
  // |locations| is an ordered vector of reference locations. Only references
  // whose location is found in [|lower_bound|, |upper_bound|) will be
  // extracted.
  Rel32ReaderX86(ConstBufferView image,
                 const AddressTranslator& translator,
                 const std::vector<offset_t>& locations,
                 offset_t lower_bound,
                 offset_t upper_bound);
  ~Rel32ReaderX86();

  // Returns the next reference, or base::nullopt if exhausted.
  base::Optional<Reference> GetNext() override;

 private:
  std::unique_ptr<RVAToOffsetTranslator> target_translator_;
  std::unique_ptr<OffsetToRVATranslator> location_translator_;
  ConstBufferView image_;
  std::vector<offset_t>::const_iterator current_;
  const std::vector<offset_t>::const_iterator last_;
  const offset_t upper_bound_;

  DISALLOW_COPY_AND_ASSIGN(Rel32ReaderX86);
};

// Writer for x86/x64 rel32 references.
class Rel32WriterX86 : ReferenceWriter {
 public:
  // |image| wraps the raw bytes of a binary in which rel32 references will be
  // written. |translator| is an suitable address translator for the binary
  // |image|.
  Rel32WriterX86(MutableBufferView image, const AddressTranslator& trans);
  ~Rel32WriterX86();

  void PutNext(Reference ref) override;

 private:
  std::unique_ptr<OffsetToRVATranslator> target_translator_;
  std::unique_ptr<OffsetToRVATranslator> location_translator_;
  MutableBufferView image_;

  DISALLOW_COPY_AND_ASSIGN(Rel32WriterX86);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_REL32_UTILS_H_
