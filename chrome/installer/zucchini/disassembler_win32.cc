// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/disassembler_win32.h"

#include <algorithm>
#include <functional>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/installer/zucchini/buffer_source.h"
#include "chrome/installer/zucchini/io_utils.h"
#include "chrome/installer/zucchini/rel32_utils.h"

namespace zucchini {

const char Win32X86Traits::kExeTypeString[] = "Windows PE x86";

const char Win32X64Traits::kExeTypeString[] = "Windows PE x64";

/******** DisassemblerWin32 ********/

// static.
// This implements the early checks in ParseHeader() without storing data.
template <class Traits>
bool DisassemblerWin32<Traits>::QuickDetect(ConstBufferView image) {
  BufferSource source(image);

  if (!source.CheckNextBytes({'M', 'Z'}))
    return false;

  uint32_t pe_header_offset = 0;
  if (!source.Skip(offsetof(pe::ImageDOSHeader, e_lfanew))
           .GetValue(&pe_header_offset) ||
      (pe_header_offset & 0x7) != 0) {
    return false;
  }

  source = BufferSource(image);
  if (!source.Skip(pe_header_offset).ConsumeBytes({'P', 'E', 0, 0}))
    return false;

  auto* coff_header = source.GetPointer<pe::ImageFileHeader>();
  if (!coff_header ||
      coff_header->size_of_optional_header <
          offsetof(typename Traits::ImageOptionalHeader, data_directory)) {
    return false;
  }

  auto* optional_header =
      source.GetPointer<typename Traits::ImageOptionalHeader>();
  if (!optional_header || optional_header->magic != Traits::kMagic)
    return false;

  return true;
}

// static
template <class Traits>
std::unique_ptr<DisassemblerWin32<Traits>> DisassemblerWin32<Traits>::Make(
    ConstBufferView image) {
  std::unique_ptr<DisassemblerWin32> disasm =
      base::MakeUnique<DisassemblerWin32>();
  if (disasm->Parse(image))
    return disasm;
  return nullptr;
}

template <class Traits>
DisassemblerWin32<Traits>::DisassemblerWin32() = default;

template <class Traits>
DisassemblerWin32<Traits>::~DisassemblerWin32() = default;

template <class Traits>
ExecutableType DisassemblerWin32<Traits>::GetExeType() const {
  return Traits::kExeType;
}

template <class Traits>
std::string DisassemblerWin32<Traits>::GetExeTypeString() const {
  return Traits::kExeTypeString;
}

template <class Traits>
std::vector<ReferenceGroup> DisassemblerWin32<Traits>::MakeReferenceGroups()
    const {
  return {
      {ReferenceTypeTraits{4, TypeTag(kRel32), PoolTag(kRel32)},
       &DisassemblerWin32::ReadRel32, &DisassemblerWin32::WriteRel32},
  };
}

template <class Traits>
std::unique_ptr<ReferenceReader> DisassemblerWin32<Traits>::ReadRel32(
    offset_t lower,
    offset_t upper) {
  ParseAndStoreRel32();
  return base::MakeUnique<Rel32ReaderX86>(image_, lower, upper,
                                          &rel32_locations_, *this);
}

template <class Traits>
std::unique_ptr<ReferenceWriter> DisassemblerWin32<Traits>::WriteRel32(
    MutableBufferView image) {
  return base::MakeUnique<Rel32WriterX86>(image, *this);
}

template <class Traits>
std::unique_ptr<RVAToOffsetTranslator>
DisassemblerWin32<Traits>::MakeRVAToOffsetTranslator() const {
  // Use lambda to wrap RVAToSection(). To avoid calling it too often, we use
  // enclosed variables |section| and |adjust| to cache results.
  struct RVAToOffset : public RVAToOffsetTranslator {
    explicit RVAToOffset(const DisassemblerWin32* self_in) : self(self_in) {}

    offset_t Convert(rva_t rva) {
      if (section == nullptr || rva < section->virtual_address ||
          rva >= section->virtual_address + std::min(section->size_of_raw_data,
                                                     section->virtual_size)) {
        section = self->RVAToSection(rva);
        if (section != nullptr &&
            rva < section->virtual_address + section->virtual_size) {
          // |rva| is in |section|. We compute adjust as such.
          adjust = section->file_offset_of_raw_data - section->virtual_address;
        } else {
          // |rva| can not be translated to offset. We make sure that the result
          // points outside image file by adding the entire size to it.
          adjust = self->size_of_image_;
        }
      }
      return rva_t(rva + adjust);
    }

    const DisassemblerWin32* self;
    const pe::ImageSectionHeader* section = nullptr;
    std::ptrdiff_t adjust = 0;
  };
  return base::MakeUnique<RVAToOffset>(this);
}

template <class Traits>
std::unique_ptr<OffsetToRVATranslator>
DisassemblerWin32<Traits>::MakeOffsetToRVATranslator() const {
  // Use lambda to wrap SectionToRVA(). To avoid calling it too often, we use
  // enclosed variables |section| and |adjust| to cache results.
  struct OffsetToRVA : public OffsetToRVATranslator {
    explicit OffsetToRVA(const DisassemblerWin32* self_in) : self(self_in) {}

    rva_t Convert(offset_t offset) {
      if (offset >= self->size_of_image_) {
        // |offset| is actually an untranslated RVA that was shifted outside the
        // image.
        return offset_t(offset - self->size_of_image_);
      }
      if (section == nullptr || offset < section->file_offset_of_raw_data ||
          offset >=
              section->file_offset_of_raw_data + section->size_of_raw_data) {
        section = self->OffsetToSection(offset);
        DCHECK_NE(section, nullptr);
        adjust = section->virtual_address - section->file_offset_of_raw_data;
      }
      return offset_t(offset + adjust);
    }

    const DisassemblerWin32* self;
    const pe::ImageSectionHeader* section = nullptr;
    std::ptrdiff_t adjust = 0;
  };
  return base::MakeUnique<OffsetToRVA>(this);
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
    const pe::ImageSectionHeader& section) {
  return (section.characteristics & kCodeCharacteristics) ==
         kCodeCharacteristics;
}

template <class Traits>
bool DisassemblerWin32<Traits>::Parse(ConstBufferView image) {
  image_ = image;
  return ParseHeader();
}

template <class Traits>
bool DisassemblerWin32<Traits>::ParseHeader() {
  BufferSource source(image_);

  if (!source.CheckNextBytes({'M', 'Z'}))
    return false;

  uint32_t pe_header_offset = 0;
  if (!source.Skip(offsetof(pe::ImageDOSHeader, e_lfanew))
           .GetValue(&pe_header_offset) ||
      (pe_header_offset & 0x7) != 0) {
    return false;
  }

  source = BufferSource(image_);
  if (!source.Skip(pe_header_offset).ConsumeBytes({'P', 'E', 0, 0}))
    return false;

  auto* coff_header = source.GetPointer<pe::ImageFileHeader>();
  if (!coff_header ||
      coff_header->size_of_optional_header <
          offsetof(typename Traits::ImageOptionalHeader, data_directory)) {
    return false;
  }

  auto* optional_header =
      source.GetPointer<typename Traits::ImageOptionalHeader>();
  if (!optional_header || optional_header->magic != Traits::kMagic)
    return false;

  const size_t kDataDirBase =
      offsetof(typename Traits::ImageOptionalHeader, data_directory);
  size_t size_of_optional_header = coff_header->size_of_optional_header;
  if (size_of_optional_header < kDataDirBase)
    return false;

  const size_t data_dir_bound =
      (size_of_optional_header - kDataDirBase) / sizeof(pe::ImageDataDirectory);
  if (optional_header->number_of_rva_and_sizes > data_dir_bound)
    return false;

  // TODO(huangs): Read base relocation table here.

  sections_count_ = coff_header->number_of_sections;
  sections_ = source.GetArray<pe::ImageSectionHeader>(sections_count_);
  if (!sections_)
    return false;

  image_base_ = optional_header->image_base;
  size_of_image_ = optional_header->size_of_image;

  // Compute file size and truncate |image_|. Note that |size_of_image_| is
  // not used because it stores the size of loaded executables in memory, and
  // not the image size on disk.
  offset_t detected_length = 0;
  section_rva_map_.resize(sections_count_);
  section_offset_map_.resize(sections_count_);
  bool has_text_section = false;
  for (int i = 0; i < sections_count_; ++i) {
    const pe::ImageSectionHeader& section = sections_[i];
    if (!image_.covers(
            {section.file_offset_of_raw_data, section.size_of_raw_data}))
      return false;
    if (IsCodeSection(section))
      has_text_section = true;
    uint32_t section_end =
        section.file_offset_of_raw_data + section.size_of_raw_data;
    detected_length = std::max(section_end, detected_length);

    section_rva_map_[i] = std::make_pair(section.virtual_address, i);
    section_offset_map_[i] = std::make_pair(section.file_offset_of_raw_data, i);
  }
  std::sort(section_offset_map_.begin(), section_offset_map_.end(),
            [](const std::pair<offset_t, int>& a, std::pair<offset_t, int>& b) {
              return a.first < b.first;
            });

  if (detected_length > image_.size())
    return false;
  if (!has_text_section)
    return false;
  image_.shrink(detected_length);

  return true;
}

template <class Traits>
bool DisassemblerWin32<Traits>::ParseAndStoreRel32() {
  if (has_parsed_rel32_)
    return true;
  has_parsed_rel32_ = true;

  // TODO(huangs): ParseAndStoreAbs32() once it's available.

  for (size_t i = 0; i < sections_count_; ++i) {
    const pe::ImageSectionHeader& section = sections_[i];
    if (!IsCodeSection(section))
      continue;

    std::ptrdiff_t from_offset_to_rva =
        section.virtual_address - section.file_offset_of_raw_data;
    rva_t start_rva = section.virtual_address;
    rva_t end_rva = start_rva + section.virtual_size;

    ConstBufferView region =
        image_[{section.file_offset_of_raw_data, section.size_of_raw_data}];
    // TODO(huangs): Initialize with |abs32_locations_|, once it's available.
    Abs32GapFinder gap_finder(image_, region, std::vector<offset_t>(),
                              Traits::kVAWidth);
    typename Traits::RelFinder finder(image_);
    // Iterate over gaps between abs32 references, to avoid collision.
    for (auto gap = gap_finder.GetNext(); gap.has_value();
         gap = gap_finder.GetNext()) {
      finder.Reset(gap.value());
      // Iterate over heuristically detected rel32 references, validate, and add
      // to |rel32_locations_|.
      for (auto rel32 = finder.GetNext(); rel32.has_value();
           rel32 = finder.GetNext()) {
        offset_t rel32_offset = offset_t(rel32->location - image_.begin());
        rva_t rel32_rva = rva_t(rel32_offset + from_offset_to_rva);
        rva_t target_rva =
            rel32_rva + 4 + *reinterpret_cast<const uint32_t*>(rel32->location);
        if (target_rva < size_of_image_ &&  // Subsumes rva != kUnassignedRVA.
            (rel32->can_point_outside_section ||
             (start_rva <= target_rva && target_rva < end_rva))) {
          finder.Accept();
          rel32_locations_.push_back(rel32_offset);
        }
      }
    }
  }
  rel32_locations_.shrink_to_fit();
  // |sections_| entries are usually sorted by offset, but there's no guarantee.
  // So sort explicitly, to be sure.
  std::sort(rel32_locations_.begin(), rel32_locations_.end());
  return true;
}

template <class Traits>
const pe::ImageSectionHeader* DisassemblerWin32<Traits>::RVAToSection(
    rva_t rva) const {
  auto it = std::upper_bound(
      section_rva_map_.begin(), section_rva_map_.end(), rva,
      [](rva_t a, const std::pair<rva_t, int>& b) { return a < b.first; });
  if (it == section_rva_map_.begin())
    return nullptr;
  --it;
  const pe::ImageSectionHeader* section = &sections_[it->second];
  return rva - it->first < section->size_of_raw_data ? section : nullptr;
}

template <class Traits>
const pe::ImageSectionHeader* DisassemblerWin32<Traits>::OffsetToSection(
    offset_t offset) const {
  auto it = std::upper_bound(section_offset_map_.begin(),
                             section_offset_map_.end(), offset,
                             [](offset_t a, const std::pair<offset_t, int>& b) {
                               return a < b.first;
                             });
  if (it == section_offset_map_.begin())
    return nullptr;
  --it;
  const pe::ImageSectionHeader* section = &sections_[it->second];
  return offset - it->first < section->size_of_raw_data ? section : nullptr;
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
  DCHECK_NE(section, nullptr);
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
