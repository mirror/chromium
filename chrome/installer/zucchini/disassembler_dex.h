// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_DEX_H_
#define CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_DEX_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/type_dex.h"

namespace zucchini {

class DisassemblerDex : public Disassembler {
 public:
  enum ReferencePool : uint8_t {
    kStringData,
    kRel,
    kMethodIdx,
    kStringIdx,
    kTypeIdx,
    kFieldIdx,
    kProtoIdx,
    kTypeList,
  };

  // We assume that types are grouped by pools.
  enum ReferenceType : uint8_t {
    kStringToData,  // kStringData

    kRel16,  // kRel
    kRel32,

    kByteCodeToMethod16,  // kMethodIdx

    kByteCodeToString16,  // kStringIdx
    kByteCodeToString32,
    kFieldToName,

    kByteCodeToType16,  // kTypeIdx
    kFieldToClass,
    kFieldToType,

    kByteCodeToField16,  // kFieldIdx

    // TODO(huangs) : Extract the following kind of pointers.
    // kMethodToClass,
    // kMethodToProto,
    // kMethodToName,
    // kProtoToShorty,
    // kProtoToReturn,
    // kProtoToParams,
    // kTypeListToType,
    // kClassDefToClass,
    // kClassDefToSuperClass,
    // kClassDefToInterfaces,
    kTypeCount
  };

  DisassemblerDex();
  ~DisassemblerDex() override;

  // Checks DEX magic header. If successful, returns true and updates
  // |*version|. Otherwise returns false.
  static bool CheckMagic(const dex::HeaderItem* header, int* version);

  // Applies quick checks to determine if |image| *may* point to the start of an
  // executable. Returns true on success.
  static bool QuickDetect(ConstBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32(offset_t lo, offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindRel16(offset_t lo, offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindByteCodeToMethod16(offset_t lo,
                                                              offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindByteCodeToString16(offset_t lo,
                                                              offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindByteCodeToString32(offset_t lo,
                                                              offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindByteCodeToType16(offset_t lo,
                                                            offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindByteCodeToField16(offset_t lo,
                                                             offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindStringToData(offset_t lo,
                                                        offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindFieldToName(offset_t lo,
                                                       offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindFieldToClass(offset_t lo,
                                                        offset_t hi);
  std::unique_ptr<ReferenceReader> MakeFindFieldToType(offset_t lo,
                                                       offset_t hi);

  std::unique_ptr<ReferenceWriter> MakeReceiveRel32(MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel16(MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveMethodIdx16(
      MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveStringIdx16(
      MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveStringIdx32(
      MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveTypeIdx16(
      MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveFieldIdx16(
      MutableBufferView image);
  std::unique_ptr<ReferenceWriter> MakeReceiveAbs32(MutableBufferView image);

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  std::vector<ReferenceGroup> MakeReferenceGroups() const override;
  bool Parse(ConstBufferView image) override;

 private:
  using MapItemMap = std::map<uint16_t, const dex::MapItem*>;

  bool ParseHeader();

  const dex::HeaderItem* header_;
  int dex_version_ = 0;
  MapItemMap map_item_map_;
  const dex::MapItem* code_items_ = nullptr;
  const dex::MapItem* method_items_ = nullptr;
  const dex::MapItem* string_items_ = nullptr;
  const dex::MapItem* type_items_ = nullptr;
  const dex::MapItem* field_items_ = nullptr;
  std::vector<const dex::CodeItem*> code_item_elements_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_DEX_H_
