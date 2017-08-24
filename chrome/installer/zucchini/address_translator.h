// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ADDRESS_TRANSLATOR_H_
#define CHROME_INSTALLER_ZUCCHINI_ADDRESS_TRANSLATOR_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// There are several ways to reason about addresses in an image:
// - Offset: Position relative to start of image.
// - VA (Virtual Address): Virtual memory address of a loaded image. This is
//   subject to relocation by the OS.
// - RVA (Relative Virtual Address): VA relative to some base address. This is
//   the preferred way to specify pointers in an image.
//
// Zucchini is primarily concerned with offsets and RVAs. Executable images like
// PE and ELF are organized into sections. Each section specifies offset and RVA
// ranges as:
//   {Offset start, offset size, RVA start, RVA size}.
// This constitutes a basic unit to translate between offsets and RVAs. Note:
// |offset size| < |RVA size| is possible. For example, the .bss section can can
// have zero-filled statically-allocated data that have no corresponding bytes
// on image (to save space). This poses a problem for Zucchini, which stores
// addresses as offsets: now we'd have "dangling RVAs" that don't map to
// offsets! Some ways to handling this are:
// 1. Ignore all dangling RVAs. This simplifies the algorithm, but also reduces
//    Zucchini's effectiveness, since some reference targets would escape
//    detection and processing.
// 2. Create distinct "fake offsets" to accommodate dangling RVAs. Image data
//    must not be read on these fake offsets, which are only valid as target
//    addresses for reference matching.
// As for |RVA size| < |offset size|, the extra portion just get ignored.
//
// Status: Zucchini implements (2) in a simple way: dangling RVAs are mapped to
// fake offsets by adding a large value. This value can be chosen as an
// exclusive upper bound of all offsets (i.e., image size). This allows them to
// be easily detected and processed as a special-case.
// TODO(huangs): Make AddressTranslator smarter: Allocate unused |offset_t|
// ranges and create "fake" units to accommodate dangling RVAs. Then
// AddressTranslator can be simplified.

// Virtual Address relative to some base address (RVA).
using rva_t = uint32_t;
// Divide by 2 to match |kOffsetBound|.
constexpr rva_t kRVABound = static_cast<rva_t>(-1) / 2;
constexpr rva_t kInvalidRVA = static_cast<rva_t>(-1);

class AddressTranslatorBuilder;
class CachedRVAToOffsetTranslator;
class CachedOffsetToRVATranslator;

// An utility to translate between offsets and RVAs in an image.
class AddressTranslator {
 public:
  // A basic unit for address translation, roughly maps to a section, but may
  // be processed (e.g., merged) as optimization.
  struct Unit {
    inline offset_t offset_end() const { return offset_begin + offset_size; }
    inline rva_t rva_end() const { return rva_begin + rva_size; }
    inline bool CoversOffset(offset_t offset) const {
      return offset >= offset_begin && offset - offset_begin < offset_size;
    }
    inline bool CoversRVA(rva_t rva) const {
      return rva >= rva_begin && rva - rva_begin < rva_size;
    }
    // Assumes valid |offset| (*cannot* be fake offset).
    inline rva_t OffsetToRVAUnsafe(offset_t offset) const {
      return offset - offset_begin + rva_begin;
    }
    // Assumes valid |rva| (*can* be danging RVA).
    inline offset_t RVAToOffsetUnsafe(rva_t rva,
                                      offset_t fake_offset_begin) const {
      rva_t delta = rva - rva_begin;
      return delta < offset_size ? delta + offset_begin
                                 : fake_offset_begin + rva;
    }
    bool HasDanglingRVA() const { return rva_size > offset_size; }
    friend bool operator==(const Unit& a, const Unit& b) {
      return a.offset_begin == b.offset_begin &&
             a.offset_size == b.offset_size && a.rva_begin == b.rva_begin &&
             a.rva_size == b.rva_size;
    }

    offset_t offset_begin;
    offset_t offset_size;
    rva_t rva_begin;
    rva_t rva_size;
  };

  AddressTranslator();
  ~AddressTranslator();

  // Returns the RVA corresponding to |offset|, or kInvalidRVA if not found.
  rva_t OffsetToRVA(offset_t offset) const;

  // Returns the offset corresponding to |rva|, or kInvalidOffset if not found.
  offset_t RVAToOffset(rva_t rva) const;

  // Returns whether a given |rva| is represented.
  bool IsValidRVA(rva_t rva) const;

  // There is no IsValidOffset(): Check for regular offsets is merely comparing
  // against image size; and fake offset check is not needed outside this class.

  // For testing.
  offset_t fake_offset_begin() const { return fake_offset_begin_; }

  const std::vector<Unit>& units_sorted_by_offset() const {
    return units_sorted_by_offset_;
  }

  const std::vector<Unit>& units_sorted_by_rva() const {
    return units_sorted_by_rva_;
  }

 private:
  friend AddressTranslatorBuilder;
  friend CachedOffsetToRVATranslator;
  friend CachedRVAToOffsetTranslator;

  // Reassigns all member variables, assumes all inputs are valid, and consumes
  // them.
  void Reset(std::vector<Unit>&& units_sorted_by_offset,
             std::vector<Unit>&& units_sorted_by_rva,
             offset_t fake_offset_begin);

  // Helper to find the Unit that contains given |offset| or |rva|. Returns null
  // if not found.
  const Unit* OffsetToUnit(offset_t offset) const;
  const Unit* RVAToUnit(rva_t rva) const;

  // Storage of Units. All offset ranges are non-empty and disjoint. Likewise
  // for all RVA ranges.
  std::vector<Unit> units_sorted_by_offset_;
  std::vector<Unit> units_sorted_by_rva_;

  // Conversion factor to translate between dangling RVAs and fake offsets.
  offset_t fake_offset_begin_;

  DISALLOW_COPY_AND_ASSIGN(AddressTranslator);
};

// A class to instantiate an AddressTranslator.
class AddressTranslatorBuilder {
 public:
  using Unit = AddressTranslator::Unit;

  enum Status {
    kSuccess = 0,
    kErrorOverflow,
    kErrorBadOverlap,
    kErrorBadOverlapDanglingRVA,
    kErrorFakeOffsetBeginTooLarge,
  };

  AddressTranslatorBuilder();
  ~AddressTranslatorBuilder();

  // All functions return "current status". Errors are permanent.

  // Attempts to add a unit, and performs basic bound checks. Empty units (i.e.m
  // |offset_size == rva_size == 0|) are allowed but are ignored.
  Status AddUnit(offset_t offset_begin,
                 offset_t offset_size,
                 rva_t rva_begin,
                 rva_t rva_size);

  // Performs consistency checks, merges overlapping Units, and writes results
  // to |trans|.
  Status Build(AddressTranslator* trans);

 protected:
  Status Fail(Status new_status);

  std::vector<Unit> units_;
  Status status_ = kSuccess;

 private:
  DISALLOW_COPY_AND_ASSIGN(AddressTranslatorBuilder);
};

// An adaptor for AddressTranslator::OffsetToRVA() that caches the last Unit
// found, to reduce the number of OffsetToUnit() calls for clustered queries.
class CachedOffsetToRVATranslator {
 public:
  // Embeds |trans| for use. Now object lifetime is tied to |trans| lifetime.
  explicit CachedOffsetToRVATranslator(const AddressTranslator& trans);
  CachedOffsetToRVATranslator(const CachedOffsetToRVATranslator&);

  rva_t Convert(offset_t offset) const;

 private:
  const AddressTranslator& trans_;
  mutable const AddressTranslator::Unit* cached_unit_ = nullptr;
};

// An adaptor for AddressTranslator::RVAToOffset() that caches the last Unit
// found, to reduce the number of RVAToUnit() calls for clustered queries.
class CachedRVAToOffsetTranslator {
 public:
  // Embeds |trans| for use. Now object lifetime is tied to |trans| lifetime.
  explicit CachedRVAToOffsetTranslator(const AddressTranslator& trans);
  CachedRVAToOffsetTranslator(const CachedRVAToOffsetTranslator&);

  offset_t Convert(rva_t rva) const;

 private:
  const AddressTranslator& trans_;
  mutable const AddressTranslator::Unit* cached_unit_ = nullptr;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ADDRESS_TRANSLATOR_H_
