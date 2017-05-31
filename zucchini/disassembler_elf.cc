// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/disassembler_elf.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <unordered_map>

#include "base/logging.h"
#include "zucchini/arm_utils.h"
#include "zucchini/io_utils.h"
#include "zucchini/ranges/algorithm.h"

namespace zucchini {

namespace {

double kARM32BitCondAlwaysDensityThreshold = 0.4;

}  // namespace

/******** ELF32Traits ********/

const char ELF32Traits::kARMExeTypeString[] = "ELF ARM";

/******** ELF64Traits ********/

const char ELF64Traits::kARMExeTypeString[] = "ELF ARM64";

/******** DisassemblerElf ********/

template <typename TRAITS>
DisassemblerElf<TRAITS>::DisassemblerElf(Region image) : Disassembler(image) {}

template <typename TRAITS>
DisassemblerElf<TRAITS>::~DisassemblerElf() = default;

// static.
template <typename TRAITS>
bool DisassemblerElf<TRAITS>::QuickDetect(Region image,
                                          e_machine_values e_machine) {
  RegionStream rs(image);

  if (!rs.Eat(0x7F, 'E', 'L', 'F'))
    return false;

  const typename TRAITS::Elf_Ehdr* header = nullptr;
  if (!rs.Seek(0) || !rs.ReadAs(&header))  // Rewind: Magic is part of header!
    return false;

  if (header->e_type != ET_EXEC && header->e_type != ET_DYN)
    return false;

  if (header->e_machine != e_machine)
    return false;

  if (header->e_version != 1)
    return false;

  if (header->e_shentsize != sizeof(typename TRAITS::Elf_Shdr))
    return false;

  if (!rs.Seek(header->e_shoff) ||
      !rs.FitsArray<typename TRAITS::Elf_Shdr>(header->e_shnum)) {
    return false;
  }

  return true;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::Parse() {
  return ParseHeader() && ParseSections();
}

template <typename TRAITS>
RVAToOffsetTranslator DisassemblerElf<TRAITS>::GetRVAToOffsetTranslator()
    const {
  const typename TRAITS::Elf_Shdr* section = nullptr;
  std::ptrdiff_t adjust = 0;

  // Use lambda to wrap RVAToSection(). To avoid calling it too often, we use
  // enclosed variables |section| and |adjust| to cache results.
  return [this, section, adjust](rva_t rva) mutable -> offset_t {
    if (section == nullptr || rva < section->sh_addr ||
        rva >= section->sh_addr + section->sh_size) {
      const typename TRAITS::Elf_Shdr* temp_section = RVAToSection(rva);
      if (temp_section == nullptr)
        return kNullOffset;
      section = temp_section;
      adjust = section->sh_offset - section->sh_addr;
    }
    return offset_t(rva + adjust);
  };
}

template <typename TRAITS>
OffsetToRVATranslator DisassemblerElf<TRAITS>::GetOffsetToRVATranslator()
    const {
  const typename TRAITS::Elf_Shdr* section = nullptr;
  std::ptrdiff_t adjust = 0;

  // Use lambda to wrap OffsetToSection(). To avoid calling it too often, we use
  // enclosed variables |section| and |adjust| to cache results.
  return [this, section, adjust](offset_t offset) mutable -> rva_t {
    if (section == nullptr || offset < section->sh_offset ||
        offset >= section->sh_offset + section->sh_size) {
      const typename TRAITS::Elf_Shdr* temp_section = OffsetToSection(offset);
      if (temp_section == nullptr)
        return kNullRva;
      section = temp_section;
      adjust = section->sh_addr - section->sh_offset;
    }
    return rva_t(offset + adjust);
  };
}

template <>
rva_t DisassemblerElf<ELF32Traits>::RelocTarget(
    typename ELF32Traits::Elf_Rel rel) const {
  // The rightmost byte of |rel.r_info| is the type. The other 3 bytes store the
  // symbol, which we ignore.
  uint32_t type = static_cast<uint32_t>(rel.r_info & 0xFF);
  if (IsElfRelTypeSupported(type))
    return rel.r_offset;
  return kNullRva;
}

template <>
rva_t DisassemblerElf<ELF64Traits>::RelocTarget(
    typename ELF64Traits::Elf_Rel rel) const {
  // The rightmost 4 bytes of r_info is the type. The other 4 bytes store the
  // symbol, which we igmore.
  uint32_t type = static_cast<uint32_t>(rel.r_info & 0xFFFFFFFF);
  if (IsElfRelTypeSupported(type)) {
    // Assume |r_offset| fits within 32-bit integer.
    if (rel.r_offset < 0x100000000ULL)
      return static_cast<rva_t>(rel.r_offset);
    // Otherwise output warning.
    static LimitedOutputStream los(std::cerr, 10);
    if (!los.full()) {
      los << "Warning: Skipping r_offset whose value exceeds 32-bits.";
      los << std::endl;
    }
  }
  return kNullRva;
}

template <typename TRAITS>
ReferenceGenerator DisassemblerElf<TRAITS>::FindRelocs(offset_t lower,
                                                       offset_t upper) {
  using Elf_Rel = typename TRAITS::Elf_Rel;
  assert(lower <= upper);
  assert(upper <= image_.size());

  if (reloc_headers_.empty())
    return [=](Reference* /* ref */) { return false; };

  // Find the relocation section at or right before |lower|.
  typename HeaderVector::iterator cur_section =
      ranges::upper_bound(reloc_headers_, lower, ranges::less(),
                          [](const typename TRAITS::Elf_Shdr* section) {
                            return section->sh_offset;
                          });
  if (cur_section != reloc_headers_.begin())
    --cur_section;

  // |lower| and |upper| do not cut across a reloc reference (e.g.,
  // Elf_Rel::r_offset), but may cut across a reloc struct (e.g. Elf_Rel)!
  // FindRelocs() emits all reloc references in [lower, upper), but needs to
  // examine the entire reloc struct for context. Knowing that |r_offset| is the
  // first entry in a reloc struct, |cursor| and |upper| are adjusted by the
  // following:
  // - If |lower| is in a reloc section, then |cursor| is chosen as |lower|
  //   aligned up to the next reloc struct, to exclude reloc struct that |lower|
  //   may cut across.
  // - If |upper| is in a reloc section, then align it up, to include reloc
  //   struct that |upper| may cut across.
  int cur_entsize = (*cur_section)->sh_entsize;
  offset_t cursor = (*cur_section)->sh_offset;
  offset_t cur_section_end = cursor + (*cur_section)->sh_size;
  if (cursor < lower)
    cursor += ceil<offset_t>(lower - cursor, cur_entsize);

  const typename TRAITS::Elf_Shdr* end_section = OffsetToSection(upper);
  if (end_section && IsRelocSection(*end_section)) {
    upper =
        end_section->sh_offset +
        ceil<offset_t>(upper - end_section->sh_offset, end_section->sh_entsize);
  }

  RVAToOffsetTranslator target_translator = GetRVAToOffsetTranslator();

  // A Generator to find all reloc references located in [|lower|, |upper|).
  // This is a nested loop restructured into a generator: the outer loop has
  // |cur_section| visiting |reloc_headers_| (sorted by |sh_offset|), and the
  // inner loop has |cursor| visiting successive reloc structs within
  // |cur_section|.
  return [=](Reference* ref) mutable {
    for (; cursor + cur_entsize <= upper; cursor += cur_entsize) {
      if (cursor >= cur_section_end) {
        ++cur_section;
        if (cur_section == reloc_headers_.end())
          return false;
        cur_entsize = (*cur_section)->sh_entsize;
        cursor = (*cur_section)->sh_offset;
        if (cursor + cur_entsize > upper)
          return false;
        cur_section_end = cursor + (*cur_section)->sh_size;
        assert(cursor < cur_section_end);
      }
      rva_t target_rva =
          RelocTarget(Region::at<Elf_Rel>(image_.begin() + cursor));
      if (target_rva == kNullRva)
        continue;
      // |target| will be used to obtain abs32 references, so we must ensure
      // that it lies inside |image_|. This is a more restrictive check than
      // IsMarked(), and therefore subsumes it.
      offset_t target = target_translator(target_rva);
      if (target == kNullOffset ||
          !CheckDataFit(target, sizeof(offset_t), image_.size())) {
        continue;
      }
      ref->target = target;
      ref->location = cursor;
      cursor += cur_entsize;
      return true;
    }
    return false;
  };
}

template <typename TRAITS>
ReferenceReceptor DisassemblerElf<TRAITS>::ReceiveRelocs() {
  OffsetToRVATranslator target_translator = GetOffsetToRVATranslator();

  return [this, target_translator](const Reference& ref) mutable {
    typename TRAITS::Elf_Rel& reloc =
        Region::at<typename TRAITS::Elf_Rel>(image_.begin(), ref.location);
    reloc.r_offset = target_translator(ref.target);
    // Leave |reloc.r_info| alone.
  };
}

template <typename TRAITS>
ReferenceGenerator DisassemblerElf<TRAITS>::FindAbs32(offset_t lower,
                                                      offset_t upper) {
  auto cur_it = ranges::lower_bound(abs32_locations_, lower);

  return [this, cur_it, upper](Reference* ref) mutable {
    while (cur_it < abs32_locations_.end() && *cur_it < upper) {
      offset_t location = *(cur_it++);
      // |target| will not be dereferenced, so we don't worry about it exceeding
      // |image_.size()| (in fact, there are valid cases where it does).
      offset_t target = image_.at<uint32_t>(location);
      // However, in rare cases, the most significant bit of |target| is set.
      // This interferes with label marking. A quick fix is to reject these.
      if (IsMarked(target)) {
        static LimitedOutputStream los(std::cerr, 10);
        if (!los.full()) {
          los << "Warning: Skipping mark-alised ELF abs32 target: ";
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

template <typename TRAITS>
ReferenceReceptor DisassemblerElf<TRAITS>::ReceiveAbs32() {
  return [this](const Reference& ref) mutable {
    Region::at<uint32_t>(image_.begin(), ref.location) = ref.target;
  };
}

template <typename TRAITS>
const typename TRAITS::Elf_Shdr* DisassemblerElf<TRAITS>::RVAToSection(
    rva_t rva) const {
  auto it = ranges::upper_bound(section_rva_map_, rva, std::less<rva_t>(),
                                ranges::key());
  if (it == section_rva_map_.begin())
    return nullptr;
  --it;
  if (rva < it->first + sections_[it->second].sh_size) {
    return sections_ + it->second;
  }
  return nullptr;
}

template <typename TRAITS>
const typename TRAITS::Elf_Shdr* DisassemblerElf<TRAITS>::OffsetToSection(
    offset_t offset) const {
  auto it = ranges::upper_bound(section_offset_map_, offset,
                                std::less<offset_t>(), ranges::key());
  if (it == section_offset_map_.begin())
    return nullptr;
  --it;
  if (offset < it->first + sections_[it->second].sh_size) {
    return sections_ + it->second;
  }
  return nullptr;
}

template <typename TRAITS>
offset_t DisassemblerElf<TRAITS>::RVAToOffset(rva_t rva) const {
  const typename TRAITS::Elf_Shdr* section = RVAToSection(rva);
  if (section == nullptr || rva >= section->sh_addr + section->sh_size) {
    return offset_t(rva + image_.size());
  }
  return offset_t(rva - section->sh_addr) + section->sh_offset;
}

template <typename TRAITS>
rva_t DisassemblerElf<TRAITS>::OffsetToRVA(offset_t offset) const {
  if (offset >= image_.size())
    return rva_t(offset - image_.size());
  const typename TRAITS::Elf_Shdr* section = OffsetToSection(offset);
  assert(section != nullptr);
  return rva_t(offset - section->sh_offset) + section->sh_addr;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::ParseHeader() {
  RegionStream rs(image_);

  if (!rs.Eat(0x7F, 'E', 'L', 'F'))
    return false;

  if (!rs.Seek(0) || !rs.ReadAs(&header_))  // Rewind: Magic is part of header!
    return false;

  if (header_->e_type != ET_EXEC && header_->e_type != ET_DYN)
    return false;

  if (header_->e_machine != ElfEM())
    return false;

  if (header_->e_version != 1)
    return false;

  if (header_->e_shentsize != sizeof(typename TRAITS::Elf_Shdr))
    return false;

  sections_count_ = header_->e_shnum;
  if (!rs.Seek(header_->e_shoff) ||
      !rs.ReadAsArray(&sections_, sections_count_)) {
    return false;
  }

  segments_count_ = header_->e_phnum;
  if (!rs.Seek(header_->e_phoff) ||
      !rs.ReadAsArray(&segments_, segments_count_)) {
    return false;
  }

  // Check string section -- even though we've stoped using them.
  Elf32_Half string_section_id = header_->e_shstrndx;
  if (string_section_id >= sections_count_)
    return false;
  size_t section_names_size = sections_[string_section_id].sh_size;
  if (section_names_size > 0) {
    // If nonempty, then last byte of string section must be null.
    const char* section_names = nullptr;
    if (!rs.Seek(sections_[string_section_id].sh_offset) ||
        !rs.ReadAsArray(&section_names, section_names_size)) {
      return false;
    }
    if (section_names[section_names_size - 1] != '\0')
      return false;
  }

  offset_t detected_length = 0;
  for (int i = 0; i < sections_count_; ++i) {
    const typename TRAITS::Elf_Shdr* section = &sections_[i];

    // Skip empty sections. These don't affect |detected_length|, and don't
    // contribute to RVA-offset mapping. Also, |sh_size == 0| triggers assert
    // failure when passed to CheckDataFit().
    if (section->sh_size == 0)
      continue;

    // Be strict: Any size overflow invalidates the file.
    if (!CheckDataFit(section->sh_offset, section->sh_size, 0x7FFFFFFF))
      return false;

    // Update |detected_length|.
    if (section->sh_type != SHT_NOBITS) {
      offset_t section_end = section->sh_offset + section->sh_size;
      detected_length = std::max(detected_length, section_end);
    }

    // Compute mappings to translate between RVA and offset. As a heuristic,
    // sections with RVA == 0 (i.e., |sh_addr == 0|) are ignored because these
    // tend to be duplicates (which cause problems during lookup), and tend to
    // be uninteresting.
    if (section->sh_addr > 0) {
      // Assume section RVA fit in int32_t, even for 64-bit architectures. If
      // assumption fails, we just skip the section (but print warning).
      if (CheckDataFit(section->sh_addr, section->sh_size, 0x7FFFFFFF)) {
        section_rva_map_.push_back(std::make_pair(section->sh_addr, i));
        section_offset_map_.push_back(std::make_pair(section->sh_offset, i));
      } else {
        std::cout << "Warning: Section " << i << " does not fit in int32_t."
                  << std::endl;
      }
    }
  }
  ranges::sort(section_rva_map_, std::less<rva_t>(), ranges::key());
  ranges::sort(section_offset_map_, std::less<offset_t>(), ranges::key());

  // Verify that |section_rva_map_| and |section_offset_map_| consist of
  // non-overlapping entries.
  for (size_t i = 1; i < section_rva_map_.size(); ++i) {
    int prev_idx = section_rva_map_[i - 1].second;
    int cur_idx = section_rva_map_[i].second;
    const typename TRAITS::Elf_Shdr* prev_section = &sections_[prev_idx];
    const typename TRAITS::Elf_Shdr* cur_section = &sections_[cur_idx];
    if (prev_section->sh_addr + prev_section->sh_size > cur_section->sh_addr) {
      std::cerr << "Warning: Section " << prev_idx
                << " RVAs overlap with section " << cur_idx << std::endl;
    }
  }
  for (size_t i = 1; i < section_offset_map_.size(); ++i) {
    int prev_idx = section_offset_map_[i - 1].second;
    int cur_idx = section_offset_map_[i].second;
    const typename TRAITS::Elf_Shdr* prev_section = &sections_[prev_idx];
    const typename TRAITS::Elf_Shdr* cur_section = &sections_[cur_idx];
    if (prev_section->sh_offset + prev_section->sh_size >
        cur_section->sh_offset) {
      std::cerr << "Warning: Section " << prev_idx
                << " offsets overlap with section " << cur_idx << std::endl;
    }
  }

  for (const typename TRAITS::Elf_Phdr* segment = segments_;
       segment != segments_ + segments_count_; ++segment) {
    offset_t segment_end = segment->p_offset + segment->p_filesz;
    detected_length = std::max(detected_length, segment_end);
  }
  offset_t section_table_end =
      header_->e_shoff + (header_->e_shnum * sizeof(typename TRAITS::Elf_Shdr));
  Elf32_Off segment_table_end =
      header_->e_phoff + (header_->e_phnum * sizeof(typename TRAITS::Elf_Phdr));
  detected_length = std::max(detected_length, section_table_end);
  detected_length = std::max(detected_length, segment_table_end);

  if (detected_length > image_.size())
    return false;
  image_.resize(detected_length);

  return true;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::IsRelocSection(
    const typename TRAITS::Elf_Shdr& section) {
  if (section.sh_size == 0)
    return false;
  if (section.sh_type == SHT_REL) {
    // Also validate |section.sh_entsize|, which gets used later.
    return section.sh_entsize == sizeof(typename TRAITS::Elf_Rel);
  }
  if (section.sh_type == SHT_RELA) {
    return section.sh_entsize == sizeof(typename TRAITS::Elf_Rela);
  }
  return false;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::IsExecSection(
    const typename TRAITS::Elf_Shdr& section) {
  return (section.sh_flags & SHF_EXECINSTR) != 0;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::ExtractInterestingSectionHeaders() {
  assert(reloc_headers_.empty());
  assert(exec_headers_.empty());
  for (Elf32_Half i = 0; i < sections_count_; ++i) {
    const typename TRAITS::Elf_Shdr* section = sections_ + i;
    if (IsRelocSection(*section))
      reloc_headers_.push_back(section);
    else if (IsExecSection(*section))
      exec_headers_.push_back(section);
  }
  auto key_fun = [](const typename TRAITS::Elf_Shdr* section) {
    return section->sh_offset;
  };
  ranges::sort(reloc_headers_, ranges::less(), key_fun);
  ranges::sort(exec_headers_, ranges::less(), key_fun);
  return true;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::GetAbs32FromRelocSections() {
  constexpr int kAbs32Width = 4;
  assert(abs32_locations_.empty());
  ReferenceGenerator relocs = FindRelocs(0, size());
  for (Reference ref; relocs(&ref);) {
    // Reject null targets and targets outside |image_|.
    if (ref.target > 0 && CheckDataFit(ref.target, kAbs32Width, image_.size()))
      abs32_locations_.push_back(ref.target);
  }
  abs32_locations_.shrink_to_fit();
  ranges::sort(abs32_locations_);
  return true;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::GetRel32FromCodeSections() {
  for (const typename TRAITS::Elf_Shdr* section : exec_headers_)
    ParseExecSection(*section);
  PostProcessRel32();
  return true;
}

template <typename TRAITS>
bool DisassemblerElf<TRAITS>::ParseSections() {
  if (!ExtractInterestingSectionHeaders())
    return false;
  if (!GetAbs32FromRelocSections())
    return false;
  if (!GetRel32FromCodeSections())
    return false;
  return true;
}

/******** DisassemblerElfX86 ********/

const ReferenceTraits DisassemblerElfX86::kGroups[] = {
    {4, kReloc, &DisassemblerElfX86::FindRelocs,
     &DisassemblerElfX86::ReceiveRelocs},
    {4, kAbs32, &DisassemblerElfX86::FindAbs32,
     &DisassemblerElfX86::ReceiveAbs32},
    {4, kRel32, &DisassemblerElfX86::FindRel32,
     &DisassemblerElfX86::ReceiveRel32},
};

DisassemblerElfX86::DisassemblerElfX86(Region image) : DisassemblerElf(image) {}

DisassemblerElfX86::~DisassemblerElfX86() = default;

// static.
bool DisassemblerElfX86::QuickDetect(Region image) {
  return Super::QuickDetect(image, EM_386);
}

ExecutableType DisassemblerElfX86::GetExeType() const {
  return EXE_TYPE_ELF_X86;
}

std::string DisassemblerElfX86::GetExeTypeString() const {
  return "ELF x86";
}

ReferenceTraits DisassemblerElfX86::GetReferenceTraits(uint8_t type) const {
  CHECK_LT(type, arraysize(kGroups));
  return kGroups[type];
}

uint8_t DisassemblerElfX86::GetReferenceTraitsCount() const {
  return kTypeCount;
}

e_machine_values DisassemblerElfX86::ElfEM() const {
  return EM_386;
}

bool DisassemblerElfX86::IsElfRelTypeSupported(uint32_t rel_type) const {
  return rel_type == R_386_RELATIVE;
}

bool DisassemblerElfX86::ParseExecSection(
    const typename ELF32Traits::Elf_Shdr& section) {
  std::ptrdiff_t from_offset_to_rva = section.sh_addr - section.sh_offset;
  RVA start_rva = section.sh_addr;
  RVA end_rva = start_rva + section.sh_size;

  Region::const_iterator begin = image_.begin() + section.sh_offset;
  Region::const_iterator end = begin + section.sh_size;
  Rel32FinderX86 finder(image_.begin(), abs32_locations_, 4);
  Rel32Finder::SearchCallback callback = [&]() -> bool {
    const Rel32FinderX86OrX64::Result rel32 = finder.GetResult();
    offset_t offset = offset_t(rel32.location - image_.begin());
    rva_t rel32_rva = rva_t(offset + from_offset_to_rva);
    rva_t target_rva = rel32_rva + 4 + Region::at<uint32_t>(rel32.location, 0);
    if (target_rva < image_.size() &&  // Subsumes rva != kUnassignedRVA.
        (rel32.can_point_outside_section ||
         (start_rva <= target_rva && target_rva < end_rva))) {
      rel32_locations_.push_back(offset);
      return true;
    }
    return false;
  };
  finder.Search(begin, end, &callback);
  return true;
}

void DisassemblerElfX86::PostProcessRel32() {
  rel32_locations_.shrink_to_fit();
  ranges::sort(rel32_locations_);
}

ReferenceGenerator DisassemblerElfX86::FindRel32(offset_t lower,
                                                 offset_t upper) {
  auto first = ranges::lower_bound(rel32_locations_, lower);
  return Rel32GeneratorX86{this, image_, first, rel32_locations_.end(), upper};
}

ReferenceReceptor DisassemblerElfX86::ReceiveRel32() {
  return Rel32ReceptorX86(this, image_);
}

/******** DisassemblerElfARM ********/

template <typename TRAITS>
DisassemblerElfARM<TRAITS>::DisassemblerElfARM(Region image) : Super(image) {}

template <typename TRAITS>
DisassemblerElfARM<TRAITS>::~DisassemblerElfARM() = default;

// static.
template <typename TRAITS>
bool DisassemblerElfARM<TRAITS>::QuickDetect(Region image) {
  return Super::QuickDetect(image, TRAITS::kARMMachineValue);
}

template <typename TRAITS>
ExecutableType DisassemblerElfARM<TRAITS>::GetExeType() const {
  return TRAITS::kARMExeType;
}

template <typename TRAITS>
std::string DisassemblerElfARM<TRAITS>::GetExeTypeString() const {
  return TRAITS::kARMExeTypeString;
}

template <typename TRAITS>
bool DisassemblerElfARM<TRAITS>::IsOffsetInExecSection(offset_t offset) {
  for (const typename TRAITS::Elf_Shdr* section : Super::exec_headers_) {
    if (offset >= section->sh_offset &&
        offset < section->sh_offset + section->sh_size) {
      return true;
    }
  }
  return false;
}

template <typename TRAITS>
e_machine_values DisassemblerElfARM<TRAITS>::ElfEM() const {
  return TRAITS::kARMMachineValue;
}

template <typename TRAITS>
void DisassemblerElfARM<TRAITS>::PostProcessRel32() {
  for (int type = 0; type < ARM32Rel32Parser::NUM_ADDR_TYPE; ++type)
    ranges::sort(rel32_locations_table_[type]);
}

template class DisassemblerElfARM<ELF32Traits>;
template class DisassemblerElfARM<ELF64Traits>;

/******** DisassemblerElfARM32 ********/

DisassemblerElfARM32::~DisassemblerElfARM32() = default;

ReferenceTraits DisassemblerElfARM32::GetReferenceTraits(uint8_t type) const {
  // Initialized on first use.
  static const std::unordered_map<uint8_t, ReferenceTraits> traits_map = {
      {ARM32ReferenceType::kReloc,
       {4, ARM32ReferenceType::kReloc, ARMReferencePool::kPoolReloc,
        &Super::FindRelocs, &Super::ReceiveRelocs}},
      {ARM32ReferenceType::kAbs32,
       {4, ARM32ReferenceType::kAbs32, ARMReferencePool::kPoolAbs32,
        &Super::FindAbs32, &Super::ReceiveAbs32}},
      {ARM32ReferenceType::kRel32_A24,
       {4, ARM32ReferenceType::kRel32_A24, ARMReferencePool::kPoolRel32,
        &DisassemblerElfARM32::FindRel32A24,
        &DisassemblerElfARM32::ReceiveRel32A24}},
      {ARM32ReferenceType::kRel32_T8,
       {2, ARM32ReferenceType::kRel32_T8, ARMReferencePool::kPoolRel32,
        &DisassemblerElfARM32::FindRel32T8,
        &DisassemblerElfARM32::ReceiveRel32T8}},
      {ARM32ReferenceType::kRel32_T11,
       {2, ARM32ReferenceType::kRel32_T11, ARMReferencePool::kPoolRel32,
        &DisassemblerElfARM32::FindRel32T11,
        &DisassemblerElfARM32::ReceiveRel32T11}},
      {ARM32ReferenceType::kRel32_T21,
       {4, ARM32ReferenceType::kRel32_T21, ARMReferencePool::kPoolRel32,
        &DisassemblerElfARM32::FindRel32T21,
        &DisassemblerElfARM32::ReceiveRel32T21}},
      {ARM32ReferenceType::kRel32_T24,
       {4, ARM32ReferenceType::kRel32_T24, ARMReferencePool::kPoolRel32,
        &DisassemblerElfARM32::FindRel32T24,
        &DisassemblerElfARM32::ReceiveRel32T24}},
  };
  return traits_map.at(type);
}

uint8_t DisassemblerElfARM32::GetReferenceTraitsCount() const {
  return ARM32ReferenceType::kTypeCount;
}

bool DisassemblerElfARM32::IsElfRelTypeSupported(uint32_t rel_type) const {
  return rel_type == R_ARM_RELATIVE;
}

bool DisassemblerElfARM32::ParseExecSection(
    const typename ELF32Traits::Elf_Shdr& section) {
  Region::const_iterator begin = image_.begin() + section.sh_offset;
  Region::const_iterator end = begin + section.sh_size;
  Rel32FinderARM32 finder(image_.begin(), abs32_locations_, 4, *this);
  finder.SetIsThumb2(IsExecSectionThumb2(section));
  Rel32Finder::SearchCallback callback = [&]() -> bool {
    const Rel32FinderARM32::Result& rel32 = finder.GetResult();
    // False detection is still possible. For example, we may misidentify ARM /
    // THUMB2 mode, or fetch non-code data. This is a quick test to eliminate
    // some of these.
    if (!IsOffsetInExecSection(rel32.target_offset)) {
      return false;
    }
    rel32_locations_table_[rel32.type].push_back(rel32.offset);
    return true;
  };
  finder.Search(begin, end, &callback);
  return true;
}

bool DisassemblerElfARM32::IsExecSectionThumb2(
    const typename ELF32Traits::Elf_Shdr& section) const {
  // ARM mode requires 4-byte alignment.
  if (section.sh_addr % 4 != 0)
    return true;
  if (section.sh_size % 4 != 0)
    return true;
  const uint8_t* first = image_.begin() + section.sh_offset;
  const uint8_t* end = first + section.sh_size;
  // Each instruction in 32-bit ARM (little-endian) looks like
  //   ?? ?? ?? X?,
  // where X specifies conditional execution. X = 0xE represents AL = "ALways
  // execute", and tends to appear very often. We use this as our main indicator
  // to discern 32-bit ARM mode from THUMB2 mode.
  size_t num = 0;
  size_t den = 0;
  for (const uint8_t* cur = first; cur < end; cur += 4) {
    uint8_t maybe_cond = Region::at(cur, 3) & 0xF0;
    if (maybe_cond == 0xE0)
      ++num;
    ++den;
  }

  std::ios old_fmt(NULL);
  old_fmt.copyfmt(std::cout);
  std::cout << "Section scan: " << num << " / " << den << " => ";
  std::cout.precision(2);
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout << (num * 100.0 / den) << "%" << std::endl;
  std::cout.copyfmt(old_fmt);
  return num < den * kARM32BitCondAlwaysDensityThreshold;
}

ReferenceGenerator DisassemblerElfARM32::FindRel32A24(offset_t lower,
                                                      offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
      *this, image_.begin(), rel32_locations_table_[ARM32Rel32Parser::ADDR_A24],
      lower, upper);
}

ReferenceReceptor DisassemblerElfARM32::ReceiveRel32A24() {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_A24>(
      *this, image_.begin());
}

ReferenceGenerator DisassemblerElfARM32::FindRel32T8(offset_t lower,
                                                     offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T8>(
      *this, image_.begin(), rel32_locations_table_[ARM32Rel32Parser::ADDR_T8],
      lower, upper);
}

ReferenceReceptor DisassemblerElfARM32::ReceiveRel32T8() {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T8>(*this,
                                                                image_.begin());
}

ReferenceGenerator DisassemblerElfARM32::FindRel32T11(offset_t lower,
                                                      offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T11>(
      *this, image_.begin(), rel32_locations_table_[ARM32Rel32Parser::ADDR_T11],
      lower, upper);
}

ReferenceReceptor DisassemblerElfARM32::ReceiveRel32T11() {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T11>(
      *this, image_.begin());
}

ReferenceGenerator DisassemblerElfARM32::FindRel32T21(offset_t lower,
                                                      offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T21>(
      *this, image_.begin(), rel32_locations_table_[ARM32Rel32Parser::ADDR_T21],
      lower, upper);
}

ReferenceReceptor DisassemblerElfARM32::ReceiveRel32T21() {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T21>(
      *this, image_.begin());
}

ReferenceGenerator DisassemblerElfARM32::FindRel32T24(offset_t lower,
                                                      offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T24>(
      *this, image_.begin(), rel32_locations_table_[ARM32Rel32Parser::ADDR_T24],
      lower, upper);
}

ReferenceReceptor DisassemblerElfARM32::ReceiveRel32T24() {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T24>(
      *this, Super::image_.begin());
}

/******** DisassemblerElfAArch64 ********/

DisassemblerElfAArch64::~DisassemblerElfAArch64() = default;

ReferenceTraits DisassemblerElfAArch64::GetReferenceTraits(uint8_t type) const {
  // Initialized on first use.
  static const std::unordered_map<uint8_t, ReferenceTraits> traits_map = {
      {AArch64ReferenceType::kReloc,
       {8, AArch64ReferenceType::kReloc, ARMReferencePool::kPoolReloc,
        &Super::FindRelocs, &Super::ReceiveRelocs}},
      {AArch64ReferenceType::kAbs32,
       {4, AArch64ReferenceType::kAbs32, ARMReferencePool::kPoolAbs32,
        &Super::FindAbs32, &Super::ReceiveAbs32}},
      {AArch64ReferenceType::kRel32_Immd14,
       {4, AArch64ReferenceType::kRel32_Immd14, ARMReferencePool::kPoolRel32,
        &DisassemblerElfAArch64::FindRel32Immd14,
        &DisassemblerElfAArch64::ReceiveRel32Immd14}},
      {AArch64ReferenceType::kRel32_Immd19,
       {4, AArch64ReferenceType::kRel32_Immd19, ARMReferencePool::kPoolRel32,
        &DisassemblerElfAArch64::FindRel32Immd19,
        &DisassemblerElfAArch64::ReceiveRel32Immd19}},
      {AArch64ReferenceType::kRel32_Immd26,
       {4, AArch64ReferenceType::kRel32_Immd26, ARMReferencePool::kPoolRel32,
        &DisassemblerElfAArch64::FindRel32Immd26,
        &DisassemblerElfAArch64::ReceiveRel32Immd26}},
  };
  return traits_map.at(type);
}

uint8_t DisassemblerElfAArch64::GetReferenceTraitsCount() const {
  return AArch64ReferenceType::kTypeCount;
}

bool DisassemblerElfAArch64::IsElfRelTypeSupported(uint32_t rel_type) const {
  // TODO(huangs): See if R_AARCH64_GLOB_DAT and R_AARCH64_JUMP_SLOT should
  // be used.
  return rel_type == R_AARCH64_RELATIVE;
}

bool DisassemblerElfAArch64::ParseExecSection(
    const typename ELF64Traits::Elf_Shdr& section) {
  Region::const_iterator begin = Super::image_.begin() + section.sh_offset;
  Region::const_iterator end = begin + section.sh_size;
  Rel32FinderAArch64 finder(Super::image_.begin(), Super::abs32_locations_,
                            *this);
  Rel32Finder::SearchCallback callback = [&]() -> bool {
    const Rel32FinderAArch64::Result& rel32 = finder.GetResult();
    // Reject illegal offsets for robustness.
    if (!IsOffsetInExecSection(rel32.target_offset))
      return false;
    rel32_locations_table_[rel32.type].push_back(rel32.offset);
    return true;
  };
  finder.Search(begin, end, &callback);
  return true;
}

ReferenceGenerator DisassemblerElfAArch64::FindRel32Immd14(offset_t lower,
                                                           offset_t upper) {
  return ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd14>(
      *this, Super::image_.begin(),
      rel32_locations_table_[AArch64Rel32Parser::ADDR_IMMD14], lower, upper);
}

ReferenceReceptor DisassemblerElfAArch64::ReceiveRel32Immd14() {
  return ARMCreateReceiveRel32<AArch64Rel32Parser::AddrTraits_Immd14>(
      *this, Super::image_.begin());
}

ReferenceGenerator DisassemblerElfAArch64::FindRel32Immd19(offset_t lower,
                                                           offset_t upper) {
  return ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd19>(
      *this, Super::image_.begin(),
      rel32_locations_table_[AArch64Rel32Parser::ADDR_IMMD19], lower, upper);
}

ReferenceReceptor DisassemblerElfAArch64::ReceiveRel32Immd19() {
  return ARMCreateReceiveRel32<AArch64Rel32Parser::AddrTraits_Immd19>(
      *this, Super::image_.begin());
}

ReferenceGenerator DisassemblerElfAArch64::FindRel32Immd26(offset_t lower,
                                                           offset_t upper) {
  return ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd26>(
      *this, Super::image_.begin(),
      rel32_locations_table_[AArch64Rel32Parser::ADDR_IMMD26], lower, upper);
}

ReferenceReceptor DisassemblerElfAArch64::ReceiveRel32Immd26() {
  return ARMCreateReceiveRel32<AArch64Rel32Parser::AddrTraits_Immd26>(
      *this, Super::image_.begin());
}

}  // namespace zucchini
