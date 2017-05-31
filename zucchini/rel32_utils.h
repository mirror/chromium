// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_REL32_UTILS_H_
#define ZUCCHINI_REL32_UTILS_H_

#include <cstdio>
#include <iostream>
#include <vector>

#include "zucchini/arm_utils.h"
#include "zucchini/image_utils.h"
#include "zucchini/io_utils.h"
#include "zucchini/ranges/algorithm.h"
#include "zucchini/region.h"

namespace zucchini {

// Given a vector of reference locations, generates x86/x64 rel32 references,
// by finding their target.
class Rel32GeneratorX86 {
 public:
  Rel32GeneratorX86(const AddressTranslator* trans,
                    Region image,
                    std::vector<offset_t>::const_iterator first,
                    std::vector<offset_t>::const_iterator last,
                    offset_t upper);
  Rel32GeneratorX86(const Rel32GeneratorX86&);  // No explicit.
  ~Rel32GeneratorX86();

  bool operator()(Reference* ref);

 private:
  RVAToOffsetTranslator target_translator_;
  OffsetToRVATranslator location_translator_;
  Region image_;
  std::vector<offset_t>::const_iterator cur_;
  const std::vector<offset_t>::const_iterator last_;
  const offset_t upper_;
};

// Receptor of x86/x64 rel32 references. When called, modifies the image file
// to encode the new target.
class Rel32ReceptorX86 {
 public:
  Rel32ReceptorX86(AddressTranslator* trans, Region image);
  Rel32ReceptorX86(const Rel32ReceptorX86&);  // No explicit.
  ~Rel32ReceptorX86();

  void operator()(const Reference& ref);

 private:
  OffsetToRVATranslator target_translator_;
  OffsetToRVATranslator location_translator_;
  Region image_;
};

// Template code to return a ReferenceGenerator to iterate over |location|
// offsets in |rel32_locations| that are constrained in |[lower, upper)|.
template <class ADDR_TRAITS>
ReferenceGenerator ARMCreateFindRel32(
    const AddressTranslator& trans,
    Region::iterator base,
    const std::vector<offset_t>& rel32_locations,
    offset_t lower,
    offset_t upper) {
  using CODE_T = typename ADDR_TRAITS::code_t;
  auto cur_it = ranges::lower_bound(rel32_locations, lower);
  OffsetToRVATranslator offset_to_rva = trans.GetOffsetToRVATranslator();
  RVAToOffsetTranslator rva_to_offset = trans.GetRVAToOffsetTranslator();
  auto rel32_end = rel32_locations.end();

  // The returned generator, which should be called repeatedly until it returns
  // false. For each call, the generator takes the next |location| in
  // |rel32_locations|, extracts the target offset, and writes both location and
  // target to |ref|.
  return [base, offset_to_rva, rva_to_offset, cur_it, upper,
          rel32_end](Reference* ref) mutable {
    while (cur_it < rel32_end && *cur_it < upper) {
      offset_t location = *(cur_it++);
      typename ADDR_TRAITS::Parser parser(offset_to_rva(location));
      CODE_T code = (parser.*ADDR_TRAITS::Fetch)(base + location);
      arm_disp_t disp;
      (parser.*ADDR_TRAITS::Decode)(code, &disp);
      offset_t target =
          rva_to_offset((parser.*ADDR_TRAITS::TargetRVAFromDisp)(disp));
      // In rare cases, the most significant bit of |target| is set. This
      // interferes with label marking. A quick fix is to reject these.
      if (IsMarked(target)) {
        static LimitedOutputStream los(std::cerr, 10);
        if (!los.full()) {
          los << "Warning: Skipping mark-alised ARM rel32 target";
          los << " (type = " << ADDR_TRAITS::addr_type << "): ";
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
  };
}

// Template code to return a ReferenceReceptor to take |ref| and transforms the
// instruction at |ref.location| so that it targets |ref.target|.
template <class ADDR_TRAITS>
ReferenceReceptor ARMCreateReceiveRel32(const AddressTranslator& trans,
                                        Region::iterator base) {
  using CODE_T = typename ADDR_TRAITS::code_t;
  OffsetToRVATranslator offset_to_rva = trans.GetOffsetToRVATranslator();

  return [base, offset_to_rva](Reference ref) mutable {
    typename ADDR_TRAITS::Parser parser(offset_to_rva(ref.location));
    CODE_T code = (parser.*ADDR_TRAITS::Fetch)(base + ref.location);
    arm_disp_t dummy_disp;
    // Dummy decode to determine opcode, so we can properly compute |disp|.
    (parser.*ADDR_TRAITS::Decode)(code, &dummy_disp);
    arm_disp_t disp =
        (parser.*ADDR_TRAITS::DispFromTargetRVA)(offset_to_rva(ref.target));
    if (!(parser.*ADDR_TRAITS::Encode)(disp, &code)) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%08X: %0*X <= %08X = %08X! Dummy: %08X",
               ref.location, static_cast<int>(sizeof(CODE_T)) * 2, code,
               static_cast<uint32_t>(disp), offset_to_rva(ref.target),
               dummy_disp);
      std::cout << "Write error: " << buf << std::endl;
      return;
    }
    (parser.*ADDR_TRAITS::Store)(base + ref.location, code);
  };
}

// Type for specialized versions of ARMCopyDisplacement().
typedef bool (*ARMCopyDisplacementFun)(rva_t src_rva,
                                       Region::const_iterator src_it,
                                       rva_t dst_rva,
                                       Region::iterator dst_it);

// Template code to make |*dst_it| similar to |*src_it| (both assumed to point
// to rel32 instructions of type ADDR_TRAITS) by copying the displacement (i.e.,
// payload bits) from |src_it| to |dst_it|. If successful, updates |*dst_it|,
// and returns true. Otherwise returns false.
template <class ADDR_TRAITS>
bool ARMCopyDisplacement(rva_t src_rva,
                         Region::const_iterator src_it,
                         rva_t dst_rva,
                         Region::iterator dst_it) {
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

#endif  // ZUCCHINI_REL32_UTILS_H_
