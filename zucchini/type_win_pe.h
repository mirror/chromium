// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_TYPE_WIN_PE_H_
#define ZUCCHINI_TYPE_WIN_PE_H_

#include <cstddef>
#include <cstdint>

namespace zucchini {

// Taken from winnt.h
namespace pe {
#pragma pack(push, 1)  // Supported by MSVC and GCC. Ensures no gaps in packing.

constexpr size_t kImageNumberOfDirectoryEntries = 16;

constexpr uint32_t kImageScnMemExecute = 0x20000000;
constexpr uint32_t kImageScnMemRead = 0x40000000;

struct ImageSectionHeader {
  char name[8];
  uint32_t virtual_size;
  uint32_t virtual_address;
  uint32_t size_of_raw_data;
  uint32_t file_offset_of_raw_data;
  uint32_t pointer_to_relocations;   // Always zero in an image.
  uint32_t pointer_to_line_numbers;  // Always zero in an image.
  uint16_t number_of_relocations;    // Always zero in an image.
  uint16_t number_of_line_numbers;   // Always zero in an image.
  uint32_t characteristics;
};

struct ImageDataDirectory {
  uint32_t virtual_address;
  uint32_t size;
};

struct ImageFileHeader {
  uint16_t machine;
  uint16_t number_of_sections;
  uint32_t time_date_stamp;
  uint32_t pointer_to_symbol_table;
  uint32_t number_of_symbols;
  uint16_t size_of_optional_header;
  uint16_t characteristics;
};

struct ImageOptionalHeader {
  uint16_t magic; /* 0x10b or 0x107 */ /* 0x00 */
  uint8_t major_linker_version;
  uint8_t minor_linker_version;
  uint32_t size_of_code;
  uint32_t size_of_initialized_data;
  uint32_t size_of_uninitialized_data;
  uint32_t address_of_entry_point; /* 0x10 */
  uint32_t base_of_code;
  uint32_t base_of_data;

  uint32_t image_base;
  uint32_t section_alignment; /* 0x20 */
  uint32_t file_alignment;
  uint16_t major_operating_system_version;
  uint16_t minor_operating_system_version;
  uint16_t major_image_version;
  uint16_t minor_image_version;
  uint16_t major_subsystem_version; /* 0x30 */
  uint16_t minor_subsystem_version;
  uint32_t win32_version_value;
  uint32_t size_of_image;
  uint32_t size_of_headers;
  uint32_t check_sum; /* 0x40 */
  uint16_t subsystem;
  uint16_t dll_characteristics;
  uint32_t size_of_stack_reserve;
  uint32_t size_of_stack_commit;
  uint32_t size_of_heap_reserve; /* 0x50 */
  uint32_t size_of_heap_commit;
  uint32_t loader_flags;
  uint32_t number_of_rva_and_sizes;
  ImageDataDirectory data_directory[kImageNumberOfDirectoryEntries]; /* 0x60 */
  /* 0xE0 */
};

struct ImageOptionalHeader64 {
  uint16_t magic; /* 0x10b or 0x107 */ /* 0x00 */
  uint8_t major_linker_version;
  uint8_t minor_linker_version;
  uint32_t size_of_code;
  uint32_t size_of_initialized_data;
  uint32_t size_of_uninitialized_data;
  uint32_t address_of_entry_point; /* 0x10 */
  uint32_t base_of_code;

  uint64_t image_base;
  uint32_t section_alignment; /* 0x20 */
  uint32_t file_alignment;
  uint16_t major_operating_system_version;
  uint16_t minor_operating_system_version;
  uint16_t major_image_version;
  uint16_t minor_image_version;
  uint16_t major_subsystem_version; /* 0x30 */
  uint16_t minor_subsystem_version;
  uint32_t win32_version_value;
  uint32_t size_of_image;
  uint32_t size_of_headers;
  uint32_t check_sum; /* 0x40 */
  uint16_t subsystem;
  uint16_t dll_characteristics;
  uint64_t size_of_stack_reserve;
  uint64_t size_of_stack_commit; /* 0x50 */
  uint64_t size_of_heap_reserve;
  uint64_t size_of_heap_commit; /* 0x60 */
  uint32_t loader_flags;
  uint32_t number_of_rva_and_sizes;
  ImageDataDirectory data_directory[kImageNumberOfDirectoryEntries]; /* 0x70 */
  /* 0xF0 */
};

#pragma pack(pop)

}  // namespace pe

// Constants and offsets gleaned from WINNT.H and various articles on the
// format of Windows PE executables.

// This is FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew):
constexpr size_t kOffsetOfNewExeHeader = 0x3C;
constexpr size_t kIndexOfBaseRelocationTable = 5;

constexpr char const* kTextSectionName = ".text";
const uint32_t kCodeCharacteristics =
    pe::kImageScnMemExecute | pe::kImageScnMemRead;

}  // namespace zucchini

#endif  // ZUCCHINI_TYPE_WIN_PE_H_
