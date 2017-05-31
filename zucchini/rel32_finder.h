// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_REL32_FINDER_H_
#define ZUCCHINI_REL32_FINDER_H_

#include <functional>
#include <vector>

#include "base/macros.h"
#include "zucchini/arm_utils.h"
#include "zucchini/image_utils.h"
#include "zucchini/region.h"

namespace zucchini {

// A class to parse executable bytes of an image to find rel32 locations.
// Architecture-specific parse details are delegated to inherited classes.
class Rel32Finder {
 public:
  // Callback function for Search() to get caller to process a newly-found rel32
  // reference. The return value of the callback indicates whether the rel32
  // found is valid.
  using SearchCallback = std::function<bool()>;

  // |base| specifies the start of the image, for file offset computation. The
  // list of |abs32_locations| (whose elements have |abs32_width|) specify
  // "taboo regions" that detected rel32 references must not overlap with.
  Rel32Finder(Region::const_iterator base,
              const std::vector<offset_t>& abs32_locations,
              int abs32_width)
      : base_(base),
        abs32_locations_(abs32_locations),
        abs32_width_(abs32_width) {}
  virtual ~Rel32Finder() = default;

  // Scans |[begin, end)| for rel32 references. For each rel32 reference found,
  // calls |callback| so the caller can extract rel32 reference (by calling
  // accessors of inherited classes) and indicate whether the rel32 found is
  // valid. Internally, Search() splits |[begin, end)| into ordered and
  // contiguous "segments", which are then dispatched to SearchSegemtn().
  void Search(Region::const_iterator begin,
              Region::const_iterator end,
              SearchCallback* callback);

  // Architecture-specific rel32 reference detection, which scans executable
  // bytes given by |[seg_begin, seg_end)|. For each rel32 reference found, the
  // implementation should cache the necessary data to be retrieved via
  // accessors, then invoke |callback|, and possibly use |callback|'s return
  // value as feedback.
  virtual void SearchSegment(Region::const_iterator seg_begin,
                             Region::const_iterator seg_end,
                             SearchCallback* callback) = 0;

 protected:
  Region::const_iterator base_;

 private:
  const std::vector<offset_t>& abs32_locations_;
  int abs32_width_;

  DISALLOW_COPY_AND_ASSIGN(Rel32Finder);
};

// Parsing for X86 or X64: we perform naive scan for opcodes that have rel32 as
// an argument, and disregard instruction alignment.
class Rel32FinderX86OrX64 : public Rel32Finder {
 public:
  // Struct to store Search() results.
  struct Result {
    Region::const_iterator location;
    bool can_point_outside_section;
  };

  Rel32FinderX86OrX64(Region::const_iterator base,
                      const std::vector<offset_t>& abs32_locations,
                      int abs32_width)
      : Rel32Finder(base, abs32_locations, abs32_width) {}

  // Rel32Finder:
  void SearchSegment(Region::const_iterator seg_begin,
                     Region::const_iterator seg_end,
                     SearchCallback* callback) override = 0;

  const Result& GetResult() const { return rel32_; }

 protected:
  // Cached results.
  Result rel32_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX86OrX64);
};

// X86 instructions.
class Rel32FinderX86 : public Rel32FinderX86OrX64 {
 public:
  Rel32FinderX86(Region::const_iterator base,
                 const std::vector<offset_t>& abs32_locations,
                 int abs32_width)
      : Rel32FinderX86OrX64(base, abs32_locations, abs32_width) {}

  // Rel32Finder:
  void SearchSegment(Region::const_iterator seg_begin,
                     Region::const_iterator seg_end,
                     SearchCallback* callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX86);
};

// X64 instructions.
class Rel32FinderX64 : public Rel32FinderX86OrX64 {
 public:
  Rel32FinderX64(Region::const_iterator base,
                 const std::vector<offset_t>& abs32_locations,
                 int abs32_width)
      : Rel32FinderX86OrX64(base, abs32_locations, abs32_width) {}

  // Rel32Finder:
  void SearchSegment(Region::const_iterator seg_begin,
                     Region::const_iterator seg_end,
                     SearchCallback* callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX64);
};

// ARM32 instructions.
class Rel32FinderARM32 : public Rel32Finder {
 public:
  struct Result {
    offset_t offset;
    offset_t target_offset;
    ARM32Rel32Parser::AddrType type;
  };

  Rel32FinderARM32(Region::const_iterator base,
                   const std::vector<offset_t>& abs32_locations,
                   int abs32_width,
                   const AddressTranslator& trans);
  ~Rel32FinderARM32() override;

  // Naively assume that an entire section would either be in ARM mode or all in
  // THUMB2 mode.
  void SetIsThumb2(bool is_thumb2) { is_thumb2_ = is_thumb2; }

  // Rel32Finder:
  void SearchSegment(Region::const_iterator seg_begin,
                     Region::const_iterator seg_end,
                     SearchCallback* callback) override;

  const Result& GetResult() const { return rel32_; }

 private:
  // Rel32 extraction, assuming segment is in ARM mode.
  void SearchSegmentA32(Region::const_iterator seg_begin,
                        Region::const_iterator seg_end,
                        SearchCallback* callback);

  // Rel32 extraction, assuming segment is in THUMB2 mode.
  void SearchSegmentT32(Region::const_iterator seg_begin,
                        Region::const_iterator seg_end,
                        SearchCallback* callback);

  bool is_thumb2_ = false;

  OffsetToRVATranslator offset_to_rva_;

  // Cached results.
  Result rel32_;

  DISALLOW_COPY_AND_ASSIGN(Rel32FinderARM32);
};

// AArch64 instructions.
class Rel32FinderAArch64 : public Rel32Finder {
 public:
  struct Result {
    offset_t offset;
    offset_t target_offset;
    AArch64Rel32Parser::AddrType type;
  };

  Rel32FinderAArch64(Region::const_iterator base,
                     const std::vector<offset_t>& abs32_locations,
                     const AddressTranslator& trans);
  ~Rel32FinderAArch64() override;

  // Rel32Finder:
  void SearchSegment(Region::const_iterator seg_begin,
                     Region::const_iterator seg_end,
                     SearchCallback* callback) override;

  const Result& GetResult() const { return rel32_; }

 private:
  OffsetToRVATranslator offset_to_rva_;

  // Cached results.
  Result rel32_;

  DISALLOW_COPY_AND_ASSIGN(Rel32FinderAArch64);
};

}  // namespace zucchini

#endif  // ZUCCHINI_REL32_FINDER_H_
