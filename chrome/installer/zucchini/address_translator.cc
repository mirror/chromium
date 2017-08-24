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
    // Convert to fake offset, and check that it is indeed fake.
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

  // Sort by RVA, then uniquefy.
  std::sort(units_.begin(), units_.end(), [](const Unit& a, const Unit& b) {
    return a.rva_begin != b.rva_begin ? a.rva_begin < b.rva_begin
                                      : a.rva_size < b.rva_size;
  });
  units_.erase(std::unique(units_.begin(), units_.end()), units_.end());

  // Scan for RVA range overlap. Validate, and merge wherever possible.
  if (units_.size() > 1) {
    // Traverse with two iterators: |slow| stays beyind and modifies Units that
    // absorb all overlapping (or tangent if suitable) Units; |fast| explores
    // new Units as candidates for consistency checks and potential merge into
    // |slow|.
    auto slow = units_.begin();
    auto fast = slow + 1;

    // All |it| with |slow| < |it| < |fast| contain garbage.
    for (; fast != units_.end(); ++fast) {
      // Comment notation: S = slow offset, F = fast offset, O = overlap offset,
      // s = slow RVA, f = fast RVA, o = overlap RVA.
      DCHECK_GE(fast->rva_begin, slow->rva_begin);
      if (slow->rva_end() < fast->rva_begin) {
        // ..ssssss..ffffff..: Disjoint: Can advance |slow|.
        ++slow;
        *slow = *fast;
        continue;
      }

      // ..ssssffff..: Tangent: Merge is optional.
      // ..sssooofff.. / ..sssooosss..: Overlap: Merge is required.
      bool merge_is_optional = slow->rva_end() == fast->rva_begin;

      // Check whether |fast| and |slow| have identical RVA -> offset shift.
      // If not, then merge cannot be resolved. Examples:
      // ..ssssffff.. -> ..SSSSFFFF..: Good, can merge.
      // ..ssssffff.. -> ..SSSS..FFFF..: Non-fatal: don't merge.
      // ..ssssffff.. -> ..FFFF..SSSS..: Non-fatal: don't merge.
      // ..ssssffff.. -> ..SSOOFF..: Fatal: Ignore for now (handled later).
      // ..sssooofff.. -> ..SSSOOOFFF..: Good, can merge.
      // ..sssooofff.. -> ..SSSSSOFFFFF..: Fatal.
      // ..sssooofff.. -> ..FFOOOOSS..: Fatal.
      // ..sssooofff.. -> ..SSSOOOF..: Good, notice |fast| has dangling RVAs.
      // ..oooooo.. -> ..OOOOOO..: Good, can merge.
      if (fast->offset_begin < slow->offset_begin ||
          fast->offset_begin - slow->offset_begin !=
              fast->rva_begin - slow->rva_begin) {
        if (merge_is_optional) {
          ++slow;
          *slow = *fast;
          continue;
        }
        return Fail(kErrorBadOverlap);
      }

      // Check whether dangling RVAs (if they exist) are consistent. Examples:
      // ..sssooofff.. -> ..SSSOOOF..: Good, can merge.
      // ..sssooosss.. -> ..SSSOOOS..: Good, can merge.
      // ..sssooofff.. -> ..SSSOO..: Good, can merge.
      // ..sssooofff.. -> ..SSSOFFF..: Fatal.
      // ..sssooosss.. -> ..SSSOOFFFF..: Fatal.
      // ..oooooo.. -> ..OOO..: Good, can merge.
      // Idea of check: Suppose |fast| has dangling RVA, then
      // |[fast->rva_start, fast->rva_start + fast->offset_start)| ->
      // |[fast->offset_start, **fast->offset_end()**)|, with remaining RVA
      // mapping to fake offsets. This means |fast->offset_end()| must be >=
      // |slow->offset_end()|, and failure to do so resluts in error. The
      // argument for |slow| haivng dranging RVA is symmetric.
      if ((fast->HasDanglingRVA() && fast->offset_end() < slow->offset_end()) ||
          (slow->HasDanglingRVA() && slow->offset_end() < fast->offset_end())) {
        if (merge_is_optional) {
          ++slow;
          *slow = *fast;
          continue;
        }
        return Fail(kErrorBadOverlapDanglingRVA);
      }

      // Merge |fast| into |slow|.
      slow->rva_size =
          std::max(slow->rva_size, fast->rva_end() - slow->rva_begin);
      slow->offset_size =
          std::max(slow->offset_size, fast->offset_end() - slow->offset_begin);
    }
    ++slow;
    units_.erase(slow, units_.end());
  }

  // After resolving RVA overlaps, any offset overlap would imply error.
  std::sort(units_.begin(), units_.end(), [](const Unit& a, const Unit& b) {
    return a.offset_begin < b.offset_begin;
  });
  if (units_.size() > 1) {
    auto slow = units_.begin();
    auto fast = slow + 1;
    for (; fast != units_.end(); ++fast) {
      if (slow->offset_end() > fast->offset_begin)
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
