// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_REL32_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_REL32_UTILS_H_

#include <stdio.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/arm_utils.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

// A visitor that emits References (locations and target) from a specified
// portion of an x86 / x64 image, given a list of valid locations.
class Rel32ReaderX86 : public ReferenceReader {
 public:
  // |image| is an image containing x86 / x64 code in [|lo|, |hi|).
  // |locations| is a sorted list of offsets of rel32 reference locations.
  // |translator| (for |image|) is embedded into |target_rva_to_offset_| and
  // |location_offset_to_rva_| for address translation, and therefore must
  // outlive |*this|.
  Rel32ReaderX86(ConstBufferView image,
                 offset_t lo,
                 offset_t hi,
                 const std::vector<offset_t>* locations,
                 const AddressTranslator& translator);
  ~Rel32ReaderX86() override;

  // Returns the next reference, or base::nullopt if exhausted.
  base::Optional<Reference> GetNext() override;

 private:
  ConstBufferView image_;
  AddressTranslator::RvaToOffsetCache target_rva_to_offset_;
  AddressTranslator::OffsetToRvaCache location_offset_to_rva_;
  const offset_t hi_;
  const std::vector<offset_t>::const_iterator last_;
  std::vector<offset_t>::const_iterator current_;

  DISALLOW_COPY_AND_ASSIGN(Rel32ReaderX86);
};

// Writer for x86 / x64 rel32 references.
class Rel32WriterX86 : public ReferenceWriter {
 public:
  // |image| wraps the raw bytes of a binary in which rel32 references will be
  // written. |translator| (for |image|) is embedded into
  // |target_offset_to_rva_| and |location_offset_to_rva_| for address
  // translation, and therefore must outlive |*this|.
  Rel32WriterX86(MutableBufferView image, const AddressTranslator& translator);
  ~Rel32WriterX86() override;

  void PutNext(Reference ref) override;

 private:
  MutableBufferView image_;
  AddressTranslator::OffsetToRvaCache target_offset_to_rva_;
  AddressTranslator::OffsetToRvaCache location_offset_to_rva_;

  DISALLOW_COPY_AND_ASSIGN(Rel32WriterX86);
};

// Template code to return a ReferenceGenerator to iterate over |location|
// offsets in |rel32_locations| that are constrained in |[lower, upper)|.
template <class ADDR_TRAITS>
std::unique_ptr<ReferenceReader> ARMCreateFindRel32(
    const AddressTranslator& translator,
    ConstBufferView::const_iterator base,
    const std::vector<offset_t>& rel32_locations,
    offset_t lower,
    offset_t upper) {
  using CODE_T = typename ADDR_TRAITS::code_t;

  // The returned generator, which should be called repeatedly until it returns
  // false. For each call, the generator takes the next |location| in
  // |rel32_locations|, extracts the target offset, and writes both location and
  // target to |ref|.
  struct ReferenceReaderImpl : public ReferenceReader {
    ReferenceReaderImpl(const AddressTranslator& translator,
                        ConstBufferView::const_iterator base_in,
                        const std::vector<offset_t>& rel32_locations,
                        offset_t lower,
                        offset_t upper_in)
        : base(base_in),
          offset_to_rva(translator),
          rva_to_offset(translator),
          upper(upper_in) {
      cur_it = std::lower_bound(rel32_locations.begin(), rel32_locations.end(),
                                lower);
      rel32_end = rel32_locations.end();
    }

    base::Optional<Reference> GetNext() override {
      while (cur_it < rel32_end && *cur_it < upper) {
        offset_t location = *(cur_it++);
        typename ADDR_TRAITS::Parser parser(offset_to_rva.Convert(location));
        CODE_T code = (parser.*ADDR_TRAITS::Fetch)(base + location);
        arm_disp_t disp;
        (parser.*ADDR_TRAITS::Decode)(code, &disp);
        offset_t target = rva_to_offset.Convert(
            (parser.*ADDR_TRAITS::TargetRvaFromDisp)(disp));
        // In rare cases, the most significant bit of |target| is set. This
        // interferes with label marking. A quick fix is to reject these.
        if (IsMarked(target)) {
          LOG(WARNING) << "Warning: Skipping mark-aliased ARM rel32 target: "
                       << " (type = " << ADDR_TRAITS::addr_type
                       << "): " << AsHex<8>(location) << " -> "
                       << AsHex<8>(target) << ".";
          continue;
        }
        return Reference{location, target};
      }
      return base::nullopt;
    }

    ConstBufferView::const_iterator base;
    AddressTranslator::OffsetToRvaCache offset_to_rva;
    AddressTranslator::RvaToOffsetCache rva_to_offset;
    std::vector<offset_t>::const_iterator cur_it;
    std::vector<offset_t>::const_iterator rel32_end;
    offset_t upper;
  };

  return base::MakeUnique<ReferenceReaderImpl>(translator, base,
                                               rel32_locations, lower, upper);
}

// Template code to return a ReferenceReceptor to take |ref| and transforms the
// instruction at |ref.location| so that it targets |ref.target|.
template <class ADDR_TRAITS>
std::unique_ptr<ReferenceWriter> ARMCreateReceiveRel32(
    const AddressTranslator& translator,
    MutableBufferView::iterator base) {
  using CODE_T = typename ADDR_TRAITS::code_t;

  struct ReferenceWriterImpl : public ReferenceWriter {
    ReferenceWriterImpl(const AddressTranslator& translator,
                        MutableBufferView::iterator base_in)
        : base(base_in), offset_to_rva(translator) {}

    void PutNext(Reference ref) override {
      typename ADDR_TRAITS::Parser parser(offset_to_rva.Convert(ref.location));
      CODE_T code = (parser.*ADDR_TRAITS::Fetch)(base + ref.location);
      arm_disp_t dummy_disp;
      // Dummy decode to determine opcode, so we can properly compute |disp|.
      (parser.*ADDR_TRAITS::Decode)(code, &dummy_disp);
      arm_disp_t disp = (parser.*ADDR_TRAITS::DispFromTargetRva)(
          offset_to_rva.Convert(ref.target));
      if (!(parser.*ADDR_TRAITS::Encode)(disp, &code)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%08X: %0*X <= %08X = %08X! Dummy: %08X",
                 ref.location, static_cast<int>(sizeof(CODE_T)) * 2, code,
                 static_cast<uint32_t>(disp), offset_to_rva.Convert(ref.target),
                 dummy_disp);
        LOG(ERROR) << "Write error: " << buf;
        return;
      }
      (parser.*ADDR_TRAITS::Store)(base + ref.location, code);
    }
    MutableBufferView::iterator base;
    AddressTranslator::OffsetToRvaCache offset_to_rva;
  };

  return base::MakeUnique<ReferenceWriterImpl>(translator, base);
}

// Type for specialized versions of ARMCopyDisplacement().
typedef bool (*ARMCopyDisplacementFun)(rva_t src_rva,
                                       ConstBufferView::const_iterator src_it,
                                       rva_t dst_rva,
                                       MutableBufferView::iterator dst_it);

// Template code to make |*dst_it| similar to |*src_it| (both assumed to point
// to rel32 instructions of type ADDR_TRAITS) by copying the displacement (i.e.,
// payload bits) from |src_it| to |dst_it|. If successful, updates |*dst_it|,
// and returns true. Otherwise returns false.
template <class ADDR_TRAITS>
bool ARMCopyDisplacement(rva_t src_rva,
                         ConstBufferView::const_iterator src_it,
                         rva_t dst_rva,
                         MutableBufferView::iterator dst_it) {
  using CODE_T = typename ADDR_TRAITS::code_t;
  typename ADDR_TRAITS::Parser src_parser(src_rva);
  typename ADDR_TRAITS::Parser dst_parser(dst_rva);
  CODE_T src_code = (src_parser.*ADDR_TRAITS::Fetch)(src_it);
  arm_disp_t disp = 0;
  if ((src_parser.*ADDR_TRAITS::Decode)(src_code, &disp)) {
    CODE_T dst_code = (dst_parser.*ADDR_TRAITS::Fetch)(dst_it);
    if ((dst_parser.*ADDR_TRAITS::Encode)(disp, &dst_code)) {
      (dst_parser.*ADDR_TRAITS::Store)(dst_it, dst_code);
      return true;
    }
  }
  return false;
}

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_REL32_UTILS_H_
