// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_tools.h"

#include <stddef.h>
#include <stdint.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_set>

#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/element_detection.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

status::Code ReadReferences(ConstBufferView image, bool do_dump) {
  std::unique_ptr<Disassembler> disasm = MakeDisassemblerWithoutFallback(image);
  if (!disasm) {
    std::cout << "Input file not recognized as executable." << std::endl;
    return status::kStatusInvalidOldImage;
  }

  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);
  std::cout.precision(4);
  std::cout.setf(std::ios::fixed, std::ios::floatfield);

  auto get_num_locations = [](const ReferenceGroup& group,
                              Disassembler* disasm) {
    size_t count = 0;
    auto refs = group.GetReader(disasm);
    for (auto ref = refs->GetNext(); ref; ref = refs->GetNext())
      ++count;
    return count;
  };

  auto get_num_targets = [](const ReferenceGroup& group, Disassembler* disasm) {
    std::unordered_set<offset_t> target_set;
    auto refs = group.GetReader(disasm);
    for (auto ref = refs->GetNext(); ref; ref = refs->GetNext())
      target_set.insert(ref->target);
    return target_set.size();
  };

  for (const auto& group : disasm->MakeReferenceGroups()) {
    std::cout << "Type " << int(group.type_tag().value());
    std::cout << ": Pool=" << static_cast<uint32_t>(group.pool_tag().value());
    std::cout << ", width=" << group.width();
    size_t num_locations = get_num_locations(group, disasm.get());
    std::cout << ", #locations=" << num_locations;
    size_t num_targets = get_num_targets(group, disasm.get());
    std::cout << ", #targets=" << num_targets;
    if (num_targets > 0) {
      double ratio = static_cast<double>(num_locations) / num_targets;
      std::cout << " (ratio=" << ratio << ")";
    }
    std::cout << std::endl;

    if (do_dump) {
      auto refs = group.GetReader(disasm.get());

      for (auto ref = refs->GetNext(); ref; ref = refs->GetNext()) {
        std::cout << "  " << AsHex<8>(ref->location);
        std::cout << " " << AsHex<8>(ref->target) << std::endl;
      }
    }
  }

  std::cout.copyfmt(old_fmt);
  return status::kStatusSuccess;
}

}  // namespace zucchini
