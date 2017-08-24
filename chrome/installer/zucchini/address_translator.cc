// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/address_translator.h"

#include <algorithm>
#include <utility>

#include "chrome/installer/zucchini/algorithm.h"

namespace zucchini {

/******** AddressTranslator ********/

AddressTranslator::AddressTranslator() = default;

AddressTranslator::~AddressTranslator() = default;

rva_t AddressTranslator::OffsetToRVA(offset_t offset) const {
  if (offset >= fake_offset_begin_) {
    // Handle dangling RVA: First shift it to regular RVA space.
    rva_t rva = offset - fake_offset_begin_;
    // Must be a valid RVA after shift.
    const Unit* unit = RVAToUnit(rva);
    if (!unit)
      return kInvalidRVA;
    // Convert to "fake offset", and check that it is indeed fake.
    rva_t delta = rva - unit->rva_begin;
    return delta >= unit->offset_size && delta < unit->rva_size ? rva
                                                                : kInvalidRVA;
  }
  const Unit* unit = OffsetToUnit(offset);
  return unit ? unit->OffsetToRVAUnsafe(offset) : kInvalidOffset;
}

offset_t AddressTranslator::RVAToOffset(rva_t rva) const {
  const Unit* unit = RVAToUnit(rva);
  // This also handles dangling RVA.
  return unit ? unit->RVAToOffsetUnsafe(rva, fake_offset_begin_) : kInvalidRVA;
}

bool AddressTranslator::IsValidRVA(rva_t rva) const {
  return static_cast<bool>(RVAToUnit(rva));
}

void AddressTranslator::Reset(
    std::vector<AddressTranslator::Unit>&& units_sorted_by_offset,
    std::vector<AddressTranslator::Unit>&& units_sorted_by_rva,
    offset_t fake_offset_begin) {
  units_sorted_by_offset_ = units_sorted_by_offset;
  units_sorted_by_rva_ = units_sorted_by_rva;
  fake_offset_begin_ = fake_offset_begin;
}

const AddressTranslator::Unit* AddressTranslator::OffsetToUnit(
    offset_t offset) const {
  // Finds first Unit with |offset_begin| > |offset|, rewind by 1 to find the
  // last Unit with |offset_begin| >= |offset| (if it exists).
  auto it = std::upper_bound(
      units_sorted_by_offset_.begin(), units_sorted_by_offset_.end(), offset,
      [](offset_t a, const Unit& b) { return a < b.offset_begin; });
  if (it == units_sorted_by_offset_.begin())
    return nullptr;
  --it;
  return it->CoversOffset(offset) ? &(*it) : nullptr;
}

const AddressTranslator::Unit* AddressTranslator::RVAToUnit(rva_t rva) const {
  auto it = std::upper_bound(
      units_sorted_by_rva_.begin(), units_sorted_by_rva_.end(), rva,
      [](rva_t a, const Unit& b) { return a < b.rva_begin; });
  if (it == units_sorted_by_rva_.begin())
    return nullptr;
  --it;
  return it->CoversRVA(rva) ? &(*it) : nullptr;
}

/******** AddressTranslatorBuilder ********/

AddressTranslatorBuilder::AddressTranslatorBuilder() = default;

AddressTranslatorBuilder::~AddressTranslatorBuilder() = default;

AddressTranslatorBuilder::Status AddressTranslatorBuilder::AddUnit(
    offset_t offset_begin,
    offset_t offset_size,
    rva_t rva_begin,
    rva_t rva_size) {
  if (status_ != kSuccess)
    return status_;
  // Check for overflows and reject if found.
  if (!RangeIsBounded<offset_t>(offset_begin, offset_size, kOffsetBound) ||
      !RangeIsBounded<rva_t>(rva_begin, rva_size, kRVABound)) {
    return Fail(kErrorOverflow);
  }
  // If |rva_size < offset_size|: Just shrink |offset_size| to accommodate.
  offset_size = std::min(offset_size, rva_size);

  // Now |rva_size >= offset_size|. Note that |rva_size > offset_size| is
  // allowed; these lead to dangling RVA.
  // Ignore empty units. i.e., both |rva_size == offset_size == 0|. Note
  // that units with |rva_size > 0 && offset_size == 0| are NOT empty.
  if (rva_size > 0)
    units_.push_back({offset_begin, offset_size, rva_begin, rva_size});
  return status_;
}

AddressTranslatorBuilder::Status AddressTranslatorBuilder::Build(
    AddressTranslator* trans) {
  if (status_ != kSuccess)
    return status_;

  auto mark_trash = [](Unit& unit) { unit.rva_size = 0; };
  auto is_trash = [](Unit& unit) { return unit.rva_size == 0; };

  // Sort by RVA, then uniquefy.
  std::sort(units_.begin(), units_.end(), [](const Unit& a, const Unit& b) {
    return a.rva_begin != b.rva_begin ? a.rva_begin < b.rva_begin
                                      : a.rva_size < b.rva_size;
  });
  units_.erase(std::unique(units_.begin(), units_.end()), units_.end());

  // Scan for RVA range overlap. Validate, and merge wherever possible.
  if (units_.size() > 1) {
    auto tail = units_.begin();
    auto head = tail + 1;

    // Moves |tail| to catch up to |head|, while skipping disposed Units
    auto advance_tail = [&tail, &head, &is_trash]() {
      do {
        ++tail;
      } while (tail != head && is_trash(*tail));
    };

    for (; head != units_.end(); ++head) {
      // Notations: T = tail offset, h = head offset, O = overlap offset,
      // t = tail RVA, h = head RVA, o = overlap RVA.
      DCHECK_GE(head->rva_begin, tail->rva_begin);
      if (tail->rva_end() < head->rva_begin) {
        // ..tttttt..hhhhhh..: Disjoint: Can advance |tail|.
        advance_tail();
        continue;
      }

      // ..tttthhhh..: Tangent: Merge is optional.
      // ..tttooohhh.. / ..tttooottt..: Overlap: Merge is required.
      bool merge_is_optional = tail->rva_end() == head->rva_begin;

      // Check whether |head| and |tail| have identical RVA -> offset shift.
      // If not, then merge cannot be resolved. Examples:
      // ..tttthhhh.. -> ..TTTTHHHH..: Good, can merge.
      // ..tttthhhh.. -> ..TTTT..HHHH..: Non-fatal: don't merge.
      // ..tttthhhh.. -> ..HHHH..TTTT..: Non-fatal: don't merge.
      // ..tttthhhh.. -> ..TTOOHH..: Fatal: Ignore for now (handled later).
      // ..tttooohhh.. -> ..TTTOOOHHH..: Good, can merge.
      // ..tttooohhh.. -> ..TTTTTOHHHHH..: Fatal.
      // ..tttooohhh.. -> ..HHOOOOTT..: Fatal.
      // ..tttooohhh.. -> ..TTTOOOH..: Good, notice |head| has dangling RVAs.
      // ..oooooo.. -> ..OOOOOO..: Good, can merge.
      if (head->offset_begin < tail->offset_begin ||
          head->offset_begin - tail->offset_begin !=
              head->rva_begin - tail->rva_begin) {
        if (merge_is_optional) {
          advance_tail();
          continue;
        }
        return Fail(kErrorBadOverlap);
      }

      // Check whether dangling RVAs (if they exist) are consistent. Examples:
      // ..tttooohhh.. -> ..TTTOOOH..: Good, can merge.
      // ..tttooottt.. -> ..TTTOOOT..: Good, can merge.
      // ..tttooohhh.. -> ..TTTOO..: Good, can merge.
      // ..tttooohhh.. -> ..TTTOHHH..: Fatal.
      // ..tttooottt.. -> ..TTTOOTTTT..: Fatal.
      // ..oooooo.. -> ..OOO..: Good, can merge.
      // Idea of check: if |head| has dangling RVA, then
      // |[head->rva_start, head->rva_start + head->offset_start)| ->
      // |[head->offset_start, **head->offset_end()**)|, and rest of RVA range
      // maps to fake offsets. Therefore |head->offset_end()| must be an
      // upper bound for |tail->offset_end()|
      if ((head->HasDanglingRVA() && head->offset_end() < tail->offset_end()) ||
          (tail->HasDanglingRVA() && tail->offset_end() < head->offset_end())) {
        if (merge_is_optional) {
          advance_tail();
          continue;
        }
        return Fail(kErrorBadOverlapDanglingRVA);
      }

      // Merge |head| into |tail|, then mark |head| for disposal.
      tail->rva_size =
          std::max(tail->rva_size, head->rva_end() - tail->rva_begin);
      tail->offset_size =
          std::max(tail->offset_size, head->offset_end() - tail->offset_begin);
      mark_trash(*head);
    }
  }

  // Remove disposed elements.
  units_.erase(std::remove_if(units_.begin(), units_.end(), is_trash),
               units_.end());

  // After resolving RVA overlaps, any offset overlap implies error.
  std::sort(units_.begin(), units_.end(), [](const Unit& a, const Unit& b) {
    return a.offset_begin < b.offset_begin;
  });
  if (units_.size() > 1) {
    auto tail = units_.begin();
    auto head = tail + 1;
    for (; head != units_.end(); ++head) {
      if (tail->offset_end() > head->offset_begin)
        return Fail(kErrorBadOverlap);
    }
  }

  // Compute exclusive upper bound for offset, to be passed as
  // |fake_offset_begin| to AddressTranslator. Note that empty units were
  // discarded early on, so do not contribute to this.
  offset_t offset_bound = 0;
  rva_t rva_bound = 0;
  for (const Unit& unit : units_) {
    offset_bound = std::max(offset_bound, unit.offset_end());
    rva_bound = std::max(rva_bound, unit.rva_end());
  }

  // Specific to fake offset heuristics: Compute pessimistic range and see if
  // it still fits within space of valid offsets. Note that this limits image
  // size to one half of |kOffsetBound|, and is a main drawback for the current
  // heuristic to convert dangling RVA to fake offsets.
  if (!RangeIsBounded(offset_bound, rva_bound, kOffsetBound))
    return kErrorFakeOffsetBeginTooLarge;

  // Success. Store results. |units_| is currently sorted by offset.
  std::vector<Unit> units_sorted_by_offset(units_.begin(), units_.end());

  std::sort(units_.begin(), units_.end(), [](const Unit& a, const Unit& b) {
    return a.rva_begin < b.rva_begin;
  });

  std::vector<Unit> units_sorted_by_rva(units_.begin(), units_.end());

  trans->Reset(std::move(units_sorted_by_offset),
               std::move(units_sorted_by_rva), offset_bound);
  return status_;
}

AddressTranslatorBuilder::Status AddressTranslatorBuilder::Fail(
    AddressTranslatorBuilder::Status new_status) {
  status_ = new_status;
  return status_;
}

/******** CachedOffsetToRVATranslator ********/

CachedOffsetToRVATranslator::CachedOffsetToRVATranslator(
    const AddressTranslator& trans)
    : trans_(trans) {}

CachedOffsetToRVATranslator::CachedOffsetToRVATranslator(
    const CachedOffsetToRVATranslator&) = default;

rva_t CachedOffsetToRVATranslator::Convert(offset_t offset) const {
  if (offset >= trans_.fake_offset_begin_) {
    // Rely on |trans_| to handle this special case.
    return trans_.OffsetToRVA(offset);
  }
  if (cached_unit_ && cached_unit_->CoversOffset(offset))
    return cached_unit_->OffsetToRVAUnsafe(offset);
  const AddressTranslator::Unit* unit = trans_.OffsetToUnit(offset);
  if (unit) {
    cached_unit_ = unit;
    return unit->OffsetToRVAUnsafe(offset);
  }
  return kInvalidRVA;
}

/******** CachedRVAToOffsetTranslator ********/

CachedRVAToOffsetTranslator::CachedRVAToOffsetTranslator(
    const AddressTranslator& trans)
    : trans_(trans) {}

CachedRVAToOffsetTranslator::CachedRVAToOffsetTranslator(
    const CachedRVAToOffsetTranslator&) = default;

offset_t CachedRVAToOffsetTranslator::Convert(rva_t rva) const {
  if (!cached_unit_ || !cached_unit_->CoversRVA(rva)) {
    const AddressTranslator::Unit* unit = trans_.RVAToUnit(rva);
    if (unit)
      cached_unit_ = unit;
    else
      return kInvalidOffset;
  }
  return cached_unit_->RVAToOffsetUnsafe(rva, trans_.fake_offset_begin_);
}

}  // namespace zucchini
