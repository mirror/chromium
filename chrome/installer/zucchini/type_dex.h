// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TYPE_DEX_H_
#define CHROME_INSTALLER_ZUCCHINI_TYPE_DEX_H_

#include <stdint.h>

namespace zucchini {
namespace dex {
// Contains types that models DEX executable format data structures.
// See https://source.android.com/devices/tech/dalvik/dalvik-bytecode.html

// The supported versions are 035 and 037.

enum class FormatId : uint8_t {
  b,
  c,
  f,
  h,
  i,
  l,
  m,
  n,
  s,
  t,
  x,
};

struct Instruction {
  Instruction() = default;
  constexpr Instruction(uint8_t op, uint8_t l, FormatId f, uint8_t v = 1)
      : opcode(op), layout(l), format(f), variant(v) {}

  uint8_t opcode;
  uint8_t layout;
  FormatId format;
  uint8_t variant = 1;
};

constexpr Instruction kByteCode[] = {
    {0x00, 1, FormatId::x},
    {0x01, 1, FormatId::x},
    {0x02, 2, FormatId::x},
    {0x03, 3, FormatId::x},
    {0x04, 1, FormatId::x},
    {0x05, 2, FormatId::x},
    {0x06, 3, FormatId::x},
    {0x07, 1, FormatId::x},
    {0x08, 2, FormatId::x},
    {0x09, 3, FormatId::x},
    {0x0A, 1, FormatId::x},
    {0x0B, 1, FormatId::x},
    {0x0C, 1, FormatId::x},
    {0x0D, 1, FormatId::x},
    {0x0E, 1, FormatId::x},
    {0x0F, 1, FormatId::x},
    {0x10, 1, FormatId::x},
    {0x11, 1, FormatId::x},
    {0x12, 1, FormatId::n},
    {0x13, 2, FormatId::s},
    {0x14, 3, FormatId::i},
    {0x15, 2, FormatId::h},
    {0x16, 2, FormatId::s},
    {0x17, 3, FormatId::i},
    {0x18, 5, FormatId::l},
    {0x19, 2, FormatId::h},
    {0x1A, 2, FormatId::c},
    {0x1B, 3, FormatId::c},
    {0x1C, 2, FormatId::c},
    {0x1D, 1, FormatId::x},
    {0x1E, 1, FormatId::x},
    {0x1F, 2, FormatId::c},
    {0x20, 2, FormatId::c},
    {0x21, 1, FormatId::x},
    {0x22, 2, FormatId::c},
    {0x23, 2, FormatId::c},
    {0x24, 3, FormatId::c},
    {0x25, 3, FormatId::c},
    {0x26, 3, FormatId::t},
    {0x27, 1, FormatId::x},
    {0x28, 1, FormatId::t},
    {0x29, 2, FormatId::t},
    {0x2A, 3, FormatId::t},
    {0x2B, 3, FormatId::t},
    {0x2C, 3, FormatId::t},
    {0x2D, 2, FormatId::x, 5},
    {0x32, 2, FormatId::t, 6},
    {0x38, 2, FormatId::t, 6},
    // {0x3e, 1, FormatId::x, 6}, unused
    {0x44, 2, FormatId::x, 14},
    {0x52, 2, FormatId::c, 14},
    {0x60, 2, FormatId::c, 14},
    {0x6e, 3, FormatId::c, 5},
    // {0x73, 1, FormatId::x}, unused
    {0x74, 3, FormatId::c, 5},
    // {0x79, 1, FormatId::x, 2}, unused
    {0x7b, 1, FormatId::x, 21},
    {0x90, 2, FormatId::x, 32},
    {0xb0, 1, FormatId::x, 32},
    {0xd0, 2, FormatId::s, 8},
    {0xd8, 2, FormatId::b, 11},
    // {0xe3, 1, FormatId::x, 29}, unused
};

#pragma pack(push, 1)  // Supported by MSVC and GCC. Ensures no gaps in packing.

struct HeaderItem {
  uint8_t magic[8];
  uint32_t checksum;
  uint8_t signature[20];
  uint32_t file_size;
  uint32_t header_size;
  uint32_t endian_tag;
  uint32_t link_size;
  uint32_t link_off;
  uint32_t map_off;
  uint32_t string_ids_size;
  uint32_t string_ids_off;
  uint32_t type_ids_size;
  uint32_t type_ids_off;
  uint32_t proto_ids_size;
  uint32_t proto_ids_off;
  uint32_t field_ids_size;
  uint32_t field_ids_off;
  uint32_t method_ids_size;
  uint32_t method_ids_off;
  uint32_t class_defs_size;
  uint32_t class_defs_off;
  uint32_t data_size;
  uint32_t data_off;
};

struct StringIdItem {
  uint32_t string_data_off;
};

struct TypeIdItem {
  uint32_t descriptor_idx;
};

struct ProtoIdItem {
  uint32_t shorty_idx;
  uint32_t return_type_idx;
  uint32_t parameters_off;
};

struct FieldIdItem {
  uint16_t class_idx;
  uint16_t type_idx;
  uint32_t name_idx;
};

struct MethodIdItem {
  uint16_t class_idx;
  uint16_t proto_idx;
  uint32_t name_idx;
};

struct ClassDefItem {
  uint32_t class_idx;
  uint32_t access_flags;
  uint32_t superclass_idx;
  uint32_t interfaces_off;
  uint32_t source_file_idx;
  uint32_t annotations_off;
  uint32_t class_data_off;
  uint32_t static_values_off;
};

struct CodeItem {
  uint16_t registers_size;
  uint16_t ins_size;
  uint16_t outs_size;
  uint16_t tries_size;
  uint32_t debug_info_off;
  uint32_t insns_size;
  // Variable length data follow.
};

constexpr uint32_t kMaxItemListSize = 18;

struct MapItem {
  uint16_t type;
  uint16_t unused;
  uint32_t size;
  uint32_t offset;
};

struct MapList {
  uint32_t size;
  MapItem list[kMaxItemListSize];
};

struct TypeItem {
  uint16_t type_idx;
};

struct AnnotationSetRefItem {
  uint32_t annotations_off;
};

struct AnnotationOffItem {
  uint32_t annotation_off;
};

struct TryItem {
  uint32_t start_addr;
  uint16_t insn_count;
  uint16_t handler_off;
};

constexpr uint16_t kTypeHeaderItem = 0x0000;
constexpr uint16_t kTypeStringIdItem = 0x0001;
constexpr uint16_t kTypeTypeIdItem = 0x0002;
constexpr uint16_t kTypeProtoIdItem = 0x0003;
constexpr uint16_t kTypeFieldIdItem = 0x0004;
constexpr uint16_t kTypeMethodIdItem = 0x0005;
constexpr uint16_t kTypeClassDefItem = 0x0006;
constexpr uint16_t kTypeMapList = 0x1000;
constexpr uint16_t kTypeTypeList = 0x1001;
constexpr uint16_t kTypeAnnotationSetRefList = 0x1002;
constexpr uint16_t kTypeAnnotationSetItem = 0x1003;
constexpr uint16_t kTypeClassDataItem = 0x2000;
constexpr uint16_t kTypeCodeItem = 0x2001;
constexpr uint16_t kTypeStringDataItem = 0x2002;
constexpr uint16_t kTypeDebugInfoItem = 0x2003;
constexpr uint16_t kTypeAnnotationItem = 0x2004;
constexpr uint16_t kTypeEncodedArrayItem = 0x2005;
constexpr uint16_t kTypeAnnotationsDirectoryItem = 0x2006;

#pragma pack(pop)

constexpr int kMaxUleb128Size = 5;

template <class It, class T>
It DecodeUleb128(It src, T* value) {
  uint8_t sh = 0;
  *value = 0;
  for (uint8_t i = 0; i < kMaxUleb128Size; ++i) {
    *value |= T(*src & 0x7F) << sh;
    sh += 7;
    if ((*(src++) & 0x80) == 0)
      break;
  }
  return src;
}

template <class It, class T>
It DecodeSleb128(It src, T* value) {
  uint8_t sh = 0;
  *value = 0;
  for (uint8_t i = 0; i < kMaxUleb128Size; ++i) {
    *value |= T(*src & 0x7F) << sh;
    sh += 7;
    if ((*(src++) & 0x80) == 0) {
      *value |= -(*value & (1 << (sh - 1)));  // Sign extend.
      break;
    }
  }
  return src;
}

template <class It>
It SkipLeb128(It src) {
  for (uint8_t i = 0; i < kMaxUleb128Size; ++i) {
    if ((*(src++) & 0x80) == 0)
      break;
  }
  return src;
}

}  // namespace dex
}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TYPE_DEX_H_
