// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/reloc_utils.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include "base/logging.h"
#include "chrome/installer/zucchini/algorithm.h"
#include "chrome/installer/zucchini/io_utils.h"
#include "chrome/installer/zucchini/type_win_pe.h"

namespace zucchini {

/******** RelocUnitWin32 ********/

RelocUnitWin32::RelocUnitWin32() = default;
RelocUnitWin32::RelocUnitWin32(uint8_t type_in,
                               offset_t location_in,
                               rva_t target_rva_in)
    : type(type_in), location(location_in), target_rva(target_rva_in) {}

bool operator==(const RelocUnitWin32& a, const RelocUnitWin32& b) {
  return std::tie(a.type, a.location, a.target_rva) ==
         std::tie(b.type, b.location, b.target_rva);
}

/******** RelocRvaReaderWin32 ********/

// static
bool RelocRvaReaderWin32::FindRelocBlocks(
    ConstBufferView image,
    BufferRegion reloc_region,
    std::vector<offset_t>* reloc_block_offsets) {
  CHECK_LT(reloc_region.size, kOffsetBound);
  ConstBufferView reloc_data = image[reloc_region];
  reloc_block_offsets->clear();
  while (reloc_data.size() >= sizeof(pe::RelocHeader)) {
    reloc_block_offsets->push_back(reloc_data.begin() - image.begin());
    auto size = reloc_data.read<pe::RelocHeader>(0).size;
    // |size| must be aligned to 4-bytes.
    if (size < sizeof(pe::RelocHeader) || size % 4 || size > reloc_data.size())
      return false;
    reloc_data.remove_prefix(size);
  }
  return reloc_data.empty();  // Fail if trailing data exist.
}

RelocRvaReaderWin32::RelocRvaReaderWin32(
    ConstBufferView image,
    BufferRegion reloc_region,
    const std::vector<offset_t>& reloc_block_offsets,
    offset_t lo,
    offset_t hi)
    : image_(image) {
  CHECK_LE(lo, hi);
  lo = base::checked_cast<offset_t>(reloc_region.InclusiveClamp(lo));
  hi = base::checked_cast<offset_t>(reloc_region.InclusiveClamp(hi));
  end_it_ = image_.begin() + hi;

  // By default, get GetNext() to produce empty output.
  cur_reloc_units_ = BufferSource(end_it_, 0);
  if (reloc_block_offsets.empty())
    return;

  // Find the block that contains |lo|.
  auto block_it = std::upper_bound(reloc_block_offsets.begin(),
                                   reloc_block_offsets.end(), lo);
  DCHECK(block_it != reloc_block_offsets.begin());
  --block_it;

  // Initialize |cur_reloc_units_| and |rva_hi_bits_|.
  if (!LoadRelocBlock(image_.begin() + *block_it))
    return;  // Nothing left.

  // Skip |cur_reloc_units_| to |lo|, truncating up.
  offset_t cur_reloc_units_offset = cur_reloc_units_.begin() - image_.begin();
  if (lo > cur_reloc_units_offset) {
    offset_t delta =
        ceil<offset_t>(lo - cur_reloc_units_offset, kRelocUnitSize);
    cur_reloc_units_.Skip(delta);
  }
}

RelocRvaReaderWin32::RelocRvaReaderWin32(RelocRvaReaderWin32&&) = default;

RelocRvaReaderWin32::~RelocRvaReaderWin32() = default;

// Unrolls a nested loop: outer = reloc blocks and inner = reloc entries.
base::Optional<RelocUnitWin32> RelocRvaReaderWin32::GetNext() {
  // "Outer loop" to find non-empty reloc block.
  while (cur_reloc_units_.Remaining() < kRelocUnitSize) {
    if (!LoadRelocBlock(cur_reloc_units_.end()))
      return base::nullopt;
  }
  if (end_it_ - cur_reloc_units_.begin() < kRelocUnitSize)
    return base::nullopt;
  // "Inner loop" to extract single reloc unit.
  offset_t location = cur_reloc_units_.begin() - image_.begin();
  uint16_t entry = cur_reloc_units_.read<uint16_t>(0);
  uint8_t type = static_cast<uint8_t>(entry >> 12);
  rva_t rva = rva_hi_bits_ + (entry & 0xFFF);
  cur_reloc_units_.Skip(kRelocUnitSize);
  return RelocUnitWin32{type, location, rva};
}

bool RelocRvaReaderWin32::LoadRelocBlock(
    ConstBufferView::const_iterator block_begin) {
  ConstBufferView header_buf(block_begin, sizeof(pe::RelocHeader));
  if (header_buf.end() >= end_it_ ||
      end_it_ - header_buf.end() < kRelocUnitSize) {
    return false;
  }
  const auto& header = header_buf.read<pe::RelocHeader>(0);
  rva_hi_bits_ = header.rva_hi;
  uint32_t block_size = header.size;
  DCHECK_GE(block_size, sizeof(pe::RelocHeader));
  cur_reloc_units_ = BufferSource(block_begin, block_size);
  cur_reloc_units_.Skip(sizeof(pe::RelocHeader));
  return true;
}

/******** RelocReaderWin32 ********/

RelocReaderWin32::RelocReaderWin32(RelocRvaReaderWin32&& reloc_rva_reader,
                                   uint16_t reloc_type,
                                   offset_t offset_bound,
                                   const AddressTranslator& translator)
    : reloc_rva_reader_(std::move(reloc_rva_reader)),
      reloc_type_(reloc_type),
      offset_bound_(offset_bound),
      entry_rva_to_offset_(translator) {}

RelocReaderWin32::~RelocReaderWin32() = default;

// ReferenceReader:
base::Optional<Reference> RelocReaderWin32::GetNext() {
  for (base::Optional<RelocUnitWin32> unit = reloc_rva_reader_.GetNext();
       unit.has_value(); unit = reloc_rva_reader_.GetNext()) {
    if (unit->type != reloc_type_)
      continue;
    offset_t target = entry_rva_to_offset_.Convert(unit->target_rva);
    if (target == kInvalidOffset)
      continue;
    offset_t location = unit->location;
    if (IsMarked(target)) {
      LOG(WARNING) << "Warning: Skipping mark-aliased reloc target: "
                   << AsHex<8>(location) << " -> " << AsHex<8>(target) << ".";
      continue;
    }
    // Ensures the target (abs32 reference) lies entirely within the image.
    if (target >= offset_bound_)
      continue;
    return Reference{location, target};
  }
  return base::nullopt;
}

/******** RelocWriterWin32 ********/

RelocWriterWin32::RelocWriterWin32(
    uint16_t reloc_type,
    MutableBufferView image,
    BufferRegion reloc_region,
    const std::vector<offset_t>& reloc_block_offsets,
    const AddressTranslator& translator)
    : reloc_type_(reloc_type),
      image_(image),
      reloc_region_(reloc_region),
      reloc_block_offsets_(reloc_block_offsets),
      target_offset_to_rva_(translator) {}

RelocWriterWin32::~RelocWriterWin32() = default;

void RelocWriterWin32::PutNext(Reference ref) {
  DCHECK_GE(ref.location, reloc_region_.lo());
  DCHECK_LT(ref.location, reloc_region_.hi());
  auto block_it = std::upper_bound(reloc_block_offsets_.begin(),
                                   reloc_block_offsets_.end(), ref.location);
  --block_it;
  rva_t rva_hi_bits = image_.read<pe::RelocHeader>(*block_it).rva_hi;
  rva_t target_rva = target_offset_to_rva_.Convert(ref.target);
  rva_t rva_lo_bits = target_rva - rva_hi_bits;
  DCHECK_EQ(rva_lo_bits & 0xFFF, rva_lo_bits);
  image_.write<uint16_t>(ref.location,
                         (rva_lo_bits & 0xFFF) | (reloc_type_ << 12));
}

/******** RelocReaderElf ********/

RelocReaderElf::RelocReaderElf(
    ConstBufferView image,
    Bitness bitness,
    const std::vector<SectionDimsElf>& reloc_section_dims,
    uint32_t rel_type,
    offset_t lo,
    offset_t hi,
    const AddressTranslator& translator)
    : image_(image),
      bitness_(bitness),
      reloc_section_dims_(reloc_section_dims),
      rel_type_(rel_type),
      hi_(hi),
      target_rva_to_offset_(translator) {
  // Find the relocation section at or right before |lo|.
  cur_section_dims_ = std::upper_bound(reloc_section_dims_.begin(),
                                       reloc_section_dims_.end(), lo);
  if (cur_section_dims_ != reloc_section_dims_.begin())
    --cur_section_dims_;

  // |lo| and |hi_| do not cut across a reloc reference (e.g.,
  // Elf_Rel::r_offset), but may cut across a reloc struct (e.g. Elf_Rel)!
  // GetNext() emits all reloc references in |[lo, hi_)|, but needs to examine
  // the entire reloc struct for context. Knowing that |r_offset| is the first
  // entry in a reloc struct, |cursor_| and |hi_| are adjusted by the following:
  // - If |lo| is in a reloc section, then |cursor_| is chosen as |lo| aligned
  //   \up to the next reloc struct, to exclude reloc struct that |lo| may cut
  //   across.
  // - If |hi_| is in a reloc section, then align it up, to include reloc struct
  //   that |hi_| may cut across.
  cur_entsize_ = cur_section_dims_->entsize;
  cursor_ = base::checked_cast<offset_t>(cur_section_dims_->region.offset);
  cur_section_dims_end_ =
      base::checked_cast<offset_t>(cur_section_dims_->region.hi());
  if (cursor_ < lo)
    cursor_ += ceil<offset_t>(lo - cursor_, cur_entsize_);

  auto end_section = std::upper_bound(reloc_section_dims_.begin(),
                                      reloc_section_dims_.end(), hi_);
  if (end_section != reloc_section_dims_.begin()) {
    --end_section;
    if (hi_ - end_section->region.offset < end_section->region.size) {
      offset_t end_region_offset =
          base::checked_cast<offset_t>(end_section->region.offset);
      hi_ = end_region_offset +
            ceil<offset_t>(hi_ - end_region_offset, end_section->entsize);
    }
  }
}

RelocReaderElf::~RelocReaderElf() = default;

rva_t RelocReaderElf::RelocTarget(Elf32_Rel rel) const {
  // The rightmost byte of |rel.r_info| is the type. The other 3 bytes store
  // the symbol, which we ignore.
  uint32_t type = static_cast<uint32_t>(rel.r_info & 0xFF);
  if (type == rel_type_)
    return rel.r_offset;
  return kInvalidRva;
}

rva_t RelocReaderElf::RelocTarget(Elf64_Rel rel) const {
  // The rightmost 4 bytes of |rel.r_info| is the type. The other 4 bytes
  // store the symbol, which we ignore.
  uint32_t type = static_cast<uint32_t>(rel.r_info & 0xFFFFFFFF);
  if (type == rel_type_) {
    // Assume |rel.r_offset| fits within 32-bit integer.
    if ((rel.r_offset & 0xFFFFFFFF) == rel.r_offset)
      return static_cast<rva_t>(rel.r_offset);
    // Otherwise output warning.
    LOG(WARNING) << "Warning: Skipping r_offset whose value exceeds 32-bits.";
  }
  return kInvalidRva;
}

base::Optional<Reference> RelocReaderElf::GetNext() {
  for (; cursor_ + cur_entsize_ <= hi_; cursor_ += cur_entsize_) {
    if (cursor_ >= cur_section_dims_end_) {
      ++cur_section_dims_;
      if (cur_section_dims_ == reloc_section_dims_.end())
        return base::nullopt;
      cur_entsize_ = base::checked_cast<offset_t>(cur_section_dims_->entsize);
      cursor_ = base::checked_cast<offset_t>(cur_section_dims_->region.offset);
      if (cursor_ + cur_entsize_ > hi_)
        return base::nullopt;
      cur_section_dims_end_ = cursor_ + base::checked_cast<offset_t>(
                                            cur_section_dims_->region.size);
      DCHECK_LT(cursor_, cur_section_dims_end_);
    }
    rva_t target_rva = kInvalidRva;
    // TODO(huangs): Fix RELA sections: Need to process |r_addend|.
    if (bitness_ == kBit32) {
      target_rva = RelocTarget(image_.read<Elf32_Rel>(cursor_));
    } else {
      DCHECK_EQ(kBit64, bitness_);
      target_rva = RelocTarget(image_.read<Elf64_Rel>(cursor_));
    }
    if (target_rva == kInvalidRva)
      continue;
    // |target| will be used to obtain abs32 references, so we must ensure
    // that it lies inside |image_|. This is a more restrictive check than
    // IsMarked(), and therefore subsumes it.
    offset_t target = target_rva_to_offset_.Convert(target_rva);
    if (target == kInvalidOffset || !image_.covers({target, sizeof(offset_t)}))
      continue;
    offset_t location = cursor_;
    cursor_ += cur_entsize_;
    return Reference{location, target};
  }
  return base::nullopt;
}

/******** RelocWriterElf ********/

RelocWriterElf::RelocWriterElf(MutableBufferView image,
                               Bitness bitness,
                               const AddressTranslator& translator)
    : image_(image), bitness_(bitness), target_offset_to_rva_(translator) {}

RelocWriterElf::~RelocWriterElf() = default;

void RelocWriterElf::PutNext(Reference ref) {
  if (bitness_ == kBit32) {
    image_.modify<Elf32_Rel>(ref.location).r_offset =
        target_offset_to_rva_.Convert(ref.target);
  } else {
    DCHECK_EQ(kBit64, bitness_);
    image_.modify<Elf64_Rel>(ref.location).r_offset =
        target_offset_to_rva_.Convert(ref.target);
  }
  // Leave |reloc.r_info| alone.
}

}  // namespace zucchini
