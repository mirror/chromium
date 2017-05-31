// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cassert>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "zucchini/disassembler.h"
#include "zucchini/ensemble.h"
#include "zucchini/io_utils.h"
#include "zucchini/program_detector.h"
#include "zucchini/reference_holder.h"
#include "zucchini/region.h"
#include "zucchini/zucchini.h"

namespace zucchini {

bool ReadRefs(Region image, bool do_dump) {
  std::unique_ptr<Disassembler> disasm = MakeDisassemblerWithoutFallback(image);
  if (!disasm) {
    std::cout << "Input file not recognized as executable." << std::endl;
    return true;
  }

  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);
  std::cout.precision(4);
  std::cout.setf(std::ios::fixed, std::ios::floatfield);

  auto get_num_locations = [](const ReferenceGroup& group) {
    size_t count = 0;
    ReferenceGenerator refs = group.Find();
    for (Reference ref; refs(&ref);)
      ++count;
    return count;
  };

  auto get_num_targets = [](const ReferenceGroup& group) {
    std::unordered_set<offset_t> target_set;
    ReferenceGenerator refs = group.Find();
    for (Reference ref; refs(&ref);)
      target_set.insert(ref.target);
    return target_set.size();
  };

  for (ReferenceGroup group : disasm->References()) {
    std::cout << "Type " << int(group.Type());
    std::cout << ": Pool=" << static_cast<uint32_t>(group.Pool());
    std::cout << ", width=" << group.Width();
    size_t num_locations = get_num_locations(group);
    std::cout << ", #locations=" << num_locations;
    size_t num_targets = get_num_targets(group);
    std::cout << ", #targets=" << num_targets;
    if (num_targets > 0) {
      double ratio = static_cast<double>(num_locations) / num_targets;
      std::cout << " (ratio=" << ratio << ")";
    }
    std::cout << std::endl;

    if (do_dump) {
      ReferenceGenerator refs = group.Find();
      for (Reference ref; refs(&ref);) {
        std::cout << "  " << AsHex<8>(ref.location);
        std::cout << " " << AsHex<8>(ref.target) << std::endl;
      }
    }
  }
  std::cout.copyfmt(old_fmt);
  return true;
}

bool DetectAll(Region image, std::vector<Region>* sub_image_list) {
  assert(sub_image_list);
  sub_image_list->clear();

  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);

  size_t last_out_pos = 0;
  size_t size = image.size();
  size_t total_bytes_found = 0;

  auto print_range = [](size_t pos, size_t size, const std::string& msg) {
    std::cout << "-- " << AsHex<8, size_t>(pos);
    std::cout << " +" << AsHex<8, size_t>(size);
    std::cout << ": " << msg << std::endl;
  };

  ProgramDetector program_detector(image);
  DisassemblerSingleProgramDetector detector;
  while (program_detector.GetNext(&detector)) {
    Region sub_image = detector.region();
    sub_image_list->push_back(sub_image);
    size_t pos = sub_image.begin() - image.begin();
    size_t prog_size = sub_image.size();
    if (last_out_pos < pos)
      print_range(last_out_pos, pos - last_out_pos, "?");
    print_range(pos, prog_size, detector.dis().GetExeTypeString());
    total_bytes_found += prog_size;
    last_out_pos = pos + prog_size;
  }
  if (last_out_pos < size)
    print_range(last_out_pos, size - last_out_pos, "?");

  // Print summary. Using decimal instead of hexadecimal.
  std::cout << std::endl;
  std::cout << "Detected " << total_bytes_found << "/" << size << " bytes => ";
  double percent = total_bytes_found * 100.0 / size;
  std::cout << std::fixed << std::setprecision(2) << percent << "%.";
  std::cout << std::endl;

  std::cout.copyfmt(old_fmt);
  return true;
}

bool MatchAll(const PatchOptions& opts, Region old_image, Region new_image) {
  EnsembleMatcher ensemble_matcher;
  ensemble_matcher.SetVerbose(true);
  if (!ensemble_matcher.RunMatch(old_image, new_image, opts.imposed_matches))
    return false;
  std::cout << "Found " << ensemble_matcher.GetMatches().size()
            << " nontrivial matches and " << ensemble_matcher.GetNumIdentical()
            << " identical matches." << std::endl;
  std::cout << std::endl;
  std::cout << "To impose the same matches by command line, use: " << std::endl;
  std::cout << "  -impose=";
  PrefixSep sep(",");
  for (const EnsembleMatcher::Match& match : ensemble_matcher.GetMatches())
    std::cout << sep << match;
  std::cout << std::endl;

  return true;
}

}  // namespace zucchini
