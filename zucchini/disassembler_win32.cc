// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/disassembler_win32.h"

#include <algorithm>
#include <cassert>
#include <functional>

#include "base/logging.h"
#include "zucchini/io_utils.h"
#include "zucchini/ranges/algorithm.h"

namespace zucchini {

const char Win32X86Traits::kExeTypeString[] = "Windows PE x86";

const char Win32X64Traits::kExeTypeString[] = "Windows PE x64";

template <class Traits>
DisassemblerWin32<Traits>::DisassemblerWin32(Region image)
    : Disassembler(image) {}

template <class Traits>
const ReferenceTraits DisassemblerWin32<Traits>::kGroups[] = {
    {2, kReloc, &DisassemblerWin32::FindRelocs,
     &DisassemblerWin32::ReceiveRelocs},
    {Traits::kVAWidth, kAbs32, &DisassemblerWin32::FindAbs32,
     &DisassemblerWin32::ReceiveAbs32},
    {4, kRel32, &DisassemblerWin32::FindRel32,
     &DisassemblerWin32::ReceiveRel32},
};

template <class Traits>
ReferenceGenerator DisassemblerWin32<Traits>::FindRelocs(offset_t lower,
                                                         offset_t upper) {
  ParseRelocs();

  size_t relocs_size = base_relocation_table_->size;
  // If no relocations exist, return vacuous generator. We isolate this case
  // because our generator emit results of a nested loop one (valid) item at a
  // time, and the "outer loop" parameter |cur_block| needs to be initalized.
  if (!relocs_size || reloc_blocks_.empty())
    return [](Reference* /* ref */) { return false; };

  Region::iterator relocs_start =
      image_.begin() + RVAToOffset(base_relocation_table_->virtual_address);
  Region::iterator relocs_end = relocs_start + relocs_size;

  // This requires |reloc_blocks_| to be non-empty. We filtered this case above.
  auto block_it = ranges::upper_bound(reloc_blocks_, lower);
  if (block_it != reloc_blocks_.begin())
    --block_it;

  RVAToOffsetTranslator entry_translator = GetRVAToOffsetTranslator();

  const_iterator cursor_end = std::min(relocs_end, image_.begin() + upper);
  const_iterator cur_block = image_.begin() + *block_it;
  const_iterator cursor = cur_block + 8;
  if (image_.begin() + lower > cursor) {
    cursor +=
        ceil<offset_t>((image_.begin() + lower) - cursor, sizeof(uint16_t));
  }
  rva_t page_rva = Region::at<uint32_t>(cur_block, 0);
  uint32_t size = Region::at<uint32_t>(cur_block, 4);
  assert(size >= 8);
  const_iterator end_entries = cur_block + size;

  // A Generator to visit relocation items in a list of relocation blocks.
  // |cur_block| points to the first reloc block that is greater or that
  // contains |lower|. |cursor_end| is a pointer that is used to constrain
  // generated references.
  return [=](Reference* ref) mutable {
    // |cursor_end - cursor| might not be a multiple of sizeof(uint16_t), so
    // we have to be careful in how we use |cursor_end|.
    for (; cursor + sizeof(uint16_t) <= cursor_end;
         cursor += sizeof(uint16_t)) {
      if (cursor >= end_entries) {
        cur_block = end_entries;
        cursor = cur_block + 8;
        if (cursor + sizeof(uint16_t) > cursor_end)
          return false;
        page_rva = Region::at<uint32_t>(cur_block, 0);
        uint32_t size = Region::at<uint32_t>(cur_block, 4);
        assert(size >= 8);
        end_entries = cur_block + size;
      }
      uint16_t entry = Region::at<uint16_t>(cursor, 0);
      int type = entry >> 12;
      if (type != Traits::kRelocType)
        continue;
      int offset = entry & 0xFFF;
      rva_t rva = page_rva + offset;
      offset_t target = entry_translator(rva);
      // Ensure |target| lies inside image. This subsumes IsMarked() check.
      if (!CheckDataFit(target, sizeof(offset_t), image_.size()))
        continue;
      ref->target = target;
      ref->location = offset_t(cursor - image_.begin());
      cursor += sizeof(uint16_t);
      return true;
    }
    return false;
  };
}

template <class Traits>
ReferenceGenerator DisassemblerWin32<Traits>::FindAbs32(offset_t lower,
                                                        offset_t upper) {
  GetAbs32FromRelocs();
  auto cur_it = ranges::lower_bound(abs32_locations_, lower);
  RVAToOffsetTranslator target_translator = GetRVAToOffsetTranslator();

  return [this, target_translator, cur_it, upper](Reference* ref) mutable {
    while (cur_it < abs32_locations_.end() && *cur_it < upper) {
      offset_t location = *(cur_it++);
      rva_t target_rva = AddressToRVA(
          Region::at<typename Traits::Address>(image_.begin(), location));
      offset_t target = target_translator(target_rva);
      // In rare cases, the most significant bit of |target| is set. This
      // interferes with label marking. A quick fix is to reject these.
      if (IsMarked(target)) {
        static LimitedOutputStream los(std::cerr, 10);
        if (!los.full()) {
          los << "Warning: Skipping mark-alised PE abs32 target: ";
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

template <class Traits>
ReferenceGenerator DisassemblerWin32<Traits>::FindRel32(offset_t lower,
                                                        offset_t upper) {
  ParseSections();
  auto first = ranges::lower_bound(rel32_locations_, lower);
  return Rel32GeneratorX86{this, image_, first, rel32_locations_.end(), upper};
}

template <class Traits>
ReferenceReceptor DisassemblerWin32<Traits>::ReceiveRelocs() {
  ParseRelocs();
  OffsetToRVATranslator target_translator = GetOffsetToRVATranslator();

  return [this, target_translator](const Reference& ref) mutable {
    auto block = ranges::upper_bound(reloc_blocks_, ref.location);
    --block;
    offset_t page_rva = Region::at<uint32_t>(image_.begin(), *block);

    RVA target_rva = target_translator(ref.target);
    int offset = target_rva - page_rva;
    Region::at<uint16_t>(image_.begin(), ref.location) =
        (offset & 0xFFF) | (Traits::kRelocType << 12);
  };
}

template <class Traits>
ReferenceReceptor DisassemblerWin32<Traits>::ReceiveAbs32() {
  OffsetToRVATranslator target_translator = GetOffsetToRVATranslator();

  return [this, target_translator](const Reference& ref) mutable {
    rva_t target_rva = target_translator(ref.target);
    typename Traits::Address address = RVAToAddress(target_rva);

    Region::at<typename Traits::Address>(image_.begin(), ref.location) =
        address;
  };
}

// static.
template <class Traits>
bool DisassemblerWin32<Traits>::QuickDetect(Region image) {
  RegionStream rs(image);

  if (!rs.Eat('M', 'Z'))
    return false;

  uint32_t pe_header_offset = 0;
  if (!rs.Seek(kOffsetOfNewExeHeader) || !rs.ReadTo(&pe_header_offset) ||
      (pe_header_offset & 0x7) != 0) {
    return false;
  }

  if (!rs.Seek(pe_header_offset) || !rs.Eat('P', 'E', 0, 0))
    return false;

  const pe::ImageFileHeader* coff_header = nullptr;
  if (!rs.ReadAs(&coff_header) ||
      coff_header->size_of_optional_header <
          offsetof(typename Traits::ImageOptionalHeader, data_directory)) {
    return false;
  }

  const typename Traits::ImageOptionalHeader* optional_header = nullptr;
  if (!rs.ReadAs(&optional_header) || optional_header->magic != Traits::kMagic)
    return false;

  return true;
}

template <class Traits>
ExecutableType DisassemblerWin32<Traits>::GetExeType() const {
  return Traits::kExeType;
}

template <class Traits>
std::string DisassemblerWin32<Traits>::GetExeTypeString() const {
  return Traits::kExeTypeString;
}

template <class Traits>
ReferenceTraits DisassemblerWin32<Traits>::GetReferenceTraits(
    uint8_t type) const {
  CHECK_LT(type, kTypeCount);
  return kGroups[type];
}

template <class Traits>
uint8_t DisassemblerWin32<Traits>::GetReferenceTraitsCount() const {
  return kTypeCount;
}

template <class Traits>
bool DisassemblerWin32<Traits>::Parse() {
  return ParseHeader();
}

template <class Traits>
RVAToOffsetTranslator DisassemblerWin32<Traits>::GetRVAToOffsetTranslator()
    const {
  const pe::ImageSectionHeader* section = nullptr;
  std::ptrdiff_t adjust = 0;

  // Use lambda to wrap RVAToSection(). To avoid calling it too often, we use
  // enclosed variables |section| and |adjust| to cache results.
  return [this, section, adjust](rva_t rva) mutable {
    if (section == nullptr || rva < section->virtual_address ||
        rva >= section->virtual_address +
                   std::min(section->size_of_raw_data, section->virtual_size)) {
      section = RVAToSection(rva);
      if (section != nullptr &&
          rva < section->virtual_address + section->virtual_size) {
        // |rva| is in |section|. We compute adjust as such.
        adjust = section->file_offset_of_raw_data - section->virtual_address;
      } else {
        // |rva| can not be translated to offset. We make sure that the result
        // points outside image file by adding the entire size to it.
        adjust = size_of_image_;
      }
    }
    return rva_t(rva + adjust);
  };
}

template <class Traits>
OffsetToRVATranslator DisassemblerWin32<Traits>::GetOffsetToRVATranslator()
    const {
  const pe::ImageSectionHeader* section = nullptr;
  std::ptrdiff_t adjust = 0;

  // Use lambda to wrap SectionToRVA(). To avoid calling it too often, we use
  // enclosed variables |section| and |adjust| to cache results.
  return [this, section, adjust](offset_t offset) mutable {
    if (offset >= size_of_image_) {
      // |offset| is actually an untranslated RVA that was shifted outside the
      // image.
      return offset_t(offset - size_of_image_);
    }
    if (section == nullptr || offset < section->file_offset_of_raw_data ||
        offset >=
            section->file_offset_of_raw_data + section->size_of_raw_data) {
      section = OffsetToSection(offset);
      assert(section != nullptr);
      adjust = section->virtual_address - section->file_offset_of_raw_data;
    }
    return offset_t(offset + adjust);
  };
}

template <class Traits>
const pe::ImageDataDirectory* DisassemblerWin32<Traits>::ReadDataDirectory(
    const typename Traits::ImageOptionalHeader* optional_header,
    size_t index) {
  if (index >= optional_header->number_of_rva_and_sizes)
    return nullptr;
  return &optional_header->data_directory[index];
}

template <class Traits>
bool DisassemblerWin32<Traits>::IsCodeSection(
    const pe::ImageSectionHeader* section) {
  return (section->characteristics & kCodeCharacteristics) ==
         kCodeCharacteristics;
}

template <class Traits>
bool DisassemblerWin32<Traits>::ParseHeader() {
  RegionStream rs(image_);

  if (!rs.Eat('M', 'Z'))
    return false;

  uint32_t pe_header_offset = 0;
  if (!rs.Seek(kOffsetOfNewExeHeader) || !rs.ReadTo(&pe_header_offset) ||
      (pe_header_offset & 0x7) != 0) {
    return false;
  }

  if (!rs.Seek(pe_header_offset) || !rs.Eat('P', 'E', 0, 0))
    return false;

  const pe::ImageFileHeader* coff_header = nullptr;
  if (!rs.ReadAs(&coff_header))
    return false;

  const size_t kDataDirBase =
      offsetof(typename Traits::ImageOptionalHeader, data_directory);
  size_t size_of_optional_header = coff_header->size_of_optional_header;
  if (size_of_optional_header < kDataDirBase)
    return false;

  const typename Traits::ImageOptionalHeader* optional_header = nullptr;
  if (!rs.ReadAs(&optional_header) || optional_header->magic != Traits::kMagic)
    return false;
  size_t data_dir_bound =
      (size_of_optional_header - kDataDirBase) / sizeof(pe::ImageDataDirectory);
  if (optional_header->number_of_rva_and_sizes > data_dir_bound)
    return false;

  base_relocation_table_ =
      ReadDataDirectory(optional_header, kIndexOfBaseRelocationTable);
  if (base_relocation_table_ == nullptr)
    return false;

  sections_count_ = coff_header->number_of_sections;
  image_base_ = optional_header->image_base;
  size_of_image_ = optional_header->size_of_image;

  // Sections follow the optional header.
  if (!rs.ReadAsArray(&sections_, sections_count_))
    return false;

  // Compute file size and truncate |image_|. Note that we cannot use
  // |size_of_image_|, which is size of loaded executable in memory, not the
  // actual size on disk.
  offset_t detected_length = 0;
  section_rva_map_.resize(sections_count_);
  section_offset_map_.resize(sections_count_);
  bool has_text_section = false;
  for (int i = 0; i < sections_count_; ++i) {
    const pe::ImageSectionHeader* section = &sections_[i];
    if (!CheckDataFit(section->file_offset_of_raw_data,
                      section->size_of_raw_data, image_.size())) {
      return false;
    }
    if (IsCodeSection(section))
      has_text_section = true;
    uint32_t section_end =
        section->file_offset_of_raw_data + section->size_of_raw_data;
    detected_length = std::max(section_end, detected_length);

    section_rva_map_[i] = std::make_pair(section->virtual_address, i);
    section_offset_map_[i] =
        std::make_pair(section->file_offset_of_raw_data, i);
  }
  ranges::sort(section_offset_map_, std::less<offset_t>(), ranges::key());

  if (detected_length > image_.size())
    return false;
  if (!has_text_section)
    return false;
  image_.resize(detected_length);

  return true;
}

template <class Traits>
bool DisassemblerWin32<Traits>::ParseRelocs() {
  if (!reloc_blocks_.empty())
    return true;

  size_t relocs_size = base_relocation_table_->size;
  if (relocs_size == 0)
    return false;

  const Region::iterator relocs_start =
      image_.begin() + RVAToOffset(base_relocation_table_->virtual_address);
  const Region::iterator relocs_end = relocs_start + relocs_size;

  if (relocs_start < image_.begin() || relocs_start >= image_.end() ||
      relocs_end < image_.begin() || relocs_end > image_.end())
    return false;

  Region::const_iterator block = relocs_start;
  while (block + 8 < relocs_end) {
    uint32_t size = Region::at<uint32_t>(block, 4);
    if (size < 8 || (size & 0x3) != 0)
      return false;

    Region::const_iterator end_entries = block + size;
    if (end_entries <= block || end_entries <= image_.begin() ||
        end_entries > image_.end())
      return false;
    reloc_blocks_.push_back(block - image_.begin());
    block = end_entries;
  }
  return true;
}

template <class Traits>
bool DisassemblerWin32<Traits>::GetAbs32FromRelocs() {
  if (!abs32_locations_.empty())
    return true;

  ReferenceGenerator relocs = FindRelocs(0, size());
  for (Reference ref; relocs(&ref);) {
    abs32_locations_.push_back(ref.target);
  }

  abs32_locations_.shrink_to_fit();
  ranges::sort(abs32_locations_);
  return true;
}

template <class Traits>
bool DisassemblerWin32<Traits>::ParseSections() {
  if (!rel32_locations_.empty())
    return true;

  GetAbs32FromRelocs();
  for (size_t i = 0; i < sections_count_; ++i) {
    ParseSection(sections_[i]);
  }
  return true;
}

template <class Traits>
bool DisassemblerWin32<Traits>::ParseSection(
    const pe::ImageSectionHeader& section) {
  bool isCode = IsCodeSection(&section);
  if (!isCode)
    return false;

  std::ptrdiff_t from_offset_to_rva =
      section.virtual_address - section.file_offset_of_raw_data;
  RVA start_rva = section.virtual_address;
  RVA end_rva = start_rva + section.virtual_size;

  auto begin = image_.begin() + section.file_offset_of_raw_data;
  auto end = begin + section.size_of_raw_data;
  typename Traits::RelFinder finder(image_.begin(), abs32_locations_,
                                    Traits::kVAWidth);
  Rel32Finder::SearchCallback callback = [&]() -> bool {
    const Rel32FinderX86OrX64::Result rel32 = finder.GetResult();
    offset_t offset = offset_t(rel32.location - image_.begin());
    rva_t rel32_rva = rva_t(offset + from_offset_to_rva);
    rva_t target_rva = rel32_rva + 4 + Region::at<uint32_t>(rel32.location, 0);
    if (target_rva < size_of_image_ &&  // Subsumes rva != kUnassignedRVA.
        (rel32.can_point_outside_section ||
         (start_rva <= target_rva && target_rva < end_rva))) {
      rel32_locations_.push_back(offset);
      return true;
    }
    return false;
  };
  finder.Search(begin, end, &callback);

  rel32_locations_.shrink_to_fit();
  return true;
}

template <class Traits>
const pe::ImageSectionHeader* DisassemblerWin32<Traits>::RVAToSection(
    rva_t rva) const {
  auto it = ranges::upper_bound(section_rva_map_, rva, std::less<rva_t>(),
                                ranges::key());
  if (it == section_rva_map_.begin())
    return nullptr;
  --it;
  if (rva < it->first + sections_[it->second].size_of_raw_data) {
    return sections_ + it->second;
  }
  return nullptr;
}

template <class Traits>
const pe::ImageSectionHeader* DisassemblerWin32<Traits>::OffsetToSection(
    offset_t offset) const {
  auto it = ranges::upper_bound(section_offset_map_, offset,
                                std::less<offset_t>(), ranges::key());
  if (it == section_offset_map_.begin())
    return nullptr;
  --it;
  if (offset < it->first + sections_[it->second].size_of_raw_data) {
    return sections_ + it->second;
  }
  return nullptr;
}

template <class Traits>
offset_t DisassemblerWin32<Traits>::RVAToOffset(rva_t rva) const {
  const pe::ImageSectionHeader* section = RVAToSection(rva);
  if (section == nullptr ||
      rva >= section->virtual_address + section->virtual_size) {
    return rva + size_of_image_;
  }
  return offset_t(rva - section->virtual_address) +
         section->file_offset_of_raw_data;
}

template <class Traits>
rva_t DisassemblerWin32<Traits>::OffsetToRVA(offset_t offset) const {
  if (offset >= size_of_image_)
    return rva_t(offset - size_of_image_);
  const pe::ImageSectionHeader* section = OffsetToSection(offset);
  assert(section != nullptr);
  return rva_t(offset - section->file_offset_of_raw_data) +
         section->virtual_address;
}

template <class Traits>
rva_t DisassemblerWin32<Traits>::AddressToRVA(
    typename Traits::Address address) const {
  return rva_t(address - image_base_);
}

template <class Traits>
typename Traits::Address DisassemblerWin32<Traits>::RVAToAddress(
    rva_t rva) const {
  return rva + image_base_;
}

template class DisassemblerWin32<Win32X86Traits>;
template class DisassemblerWin32<Win32X64Traits>;

}  // namespace zucchini
