// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/imposed_match_parser.h"

#include <algorithm>
#include <sstream>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

bool ParseImposedMatches(const std::string& imposed_matches,
                         ConstBufferView old_image,
                         ConstBufferView new_image,
                         ElementDetector&& detector,
                         std::ostream& out_str,
                         size_t* num_identical,
                         std::vector<ElementMatch>* matches) {
  size_t temp_num_identical = 0;
  std::vector<ElementMatch> temp_matches;

  // Parse |imposed_matches| and check bounds.
  std::istringstream iss(imposed_matches);
  bool first = true;
  iss.peek();  // Makes empty |iss| realize EOF is reached.
  while (iss && !iss.eof()) {
    // Eat delimiter.
    if (first) {
      first = false;
    } else if (!(iss >> EatChar(','))) {
      out_str << "Imposed matches have invalid delimiter." << std::endl;
      return false;
    }
    // Extract parameters for one imposed match.
    offset_t old_offset = 0U;
    size_t old_size = 0U;
    offset_t new_offset = 0U;
    size_t new_size = 0U;
    if (!(iss >> StrictUInt<offset_t>(old_offset) >> EatChar('+') >>
          StrictUInt<size_t>(old_size) >> EatChar('=') >>
          StrictUInt<offset_t>(new_offset) >> EatChar('+') >>
          StrictUInt<size_t>(new_size))) {
      out_str << "Parse error in imposed matches." << std::endl;
      return false;
    }
    // Check bounds.
    if (old_size == 0 || new_size == 0 ||
        !old_image.covers({old_offset, old_size}) ||
        !new_image.covers({new_offset, new_size})) {
      out_str << "Imposed matches have out-of-bound entry." << std::endl;
      return false;
    }
    temp_matches.push_back(
        {{{old_offset, old_size}, kExeTypeUnknown},    // Assign type later.
         {{new_offset, new_size}, kExeTypeUnknown}});  // Assign type later.
  }
  // Sort matches by "new" file offsets. This helps with overlap checks.
  std::sort(temp_matches.begin(), temp_matches.end(),
            [](const ElementMatch& match_a, const ElementMatch& match_b) {
              return match_a.new_element.offset < match_b.new_element.offset;
            });

  // Check for overlaps in "new" file.
  for (size_t i = 1; i < temp_matches.size(); ++i) {
    if (temp_matches[i - 1].new_element.hi() >
        temp_matches[i].new_element.lo()) {
      out_str << "Imposed matches have overlap in \"new\" file." << std::endl;
      return false;
    }
  }

  // Compute types and verify consistency. Remove identical matches and matches
  // where any sub-image has an unknown type.
  size_t write_idx = 0;
  for (size_t read_idx = 0; read_idx < temp_matches.size(); ++read_idx) {
    ConstBufferView old_sub_image(
        old_image[temp_matches[read_idx].old_element.region()]);
    ConstBufferView new_sub_image(
        new_image[temp_matches[read_idx].new_element.region()]);
    // Remove identical match.
    if (old_sub_image.equals(new_sub_image)) {
      ++temp_num_identical;
      continue;
    }
    // Check executable types of sub-images.
    base::Optional<Element> old_element = detector.Run(old_sub_image);
    base::Optional<Element> new_element = detector.Run(new_sub_image);
    if (!old_element || !new_element) {
      // Skip unknown types, including those mixed with known types.
      out_str << "Warning: Skipping unknown type in match: "
              << FormatElementMatch(temp_matches[read_idx]) << "." << std::endl;
      continue;
    } else if (old_element->exe_type != new_element->exe_type) {
      // Inconsistent known types are errors.
      out_str << "Inconsistent types in match: "
              << FormatElementMatch(temp_matches[read_idx]) << "." << std::endl;
      return false;
    }

    // Keep match and remove gaps.
    temp_matches[read_idx].old_element.exe_type = old_element->exe_type;
    temp_matches[read_idx].new_element.exe_type = new_element->exe_type;
    if (write_idx < read_idx)
      temp_matches[write_idx] = temp_matches[read_idx];
    ++write_idx;
  }
  temp_matches.resize(write_idx);

  *num_identical = temp_num_identical;
  matches->swap(temp_matches);
  return true;
}

std::string FormatElementMatch(const ElementMatch& match) {
  return base::StringPrintf("%zd+%zd=%zd+%zd",
                            static_cast<size_t>(match.old_element.offset),
                            static_cast<size_t>(match.old_element.size),
                            static_cast<size_t>(match.new_element.offset),
                            static_cast<size_t>(match.new_element.size));
}

}  // namespace zucchini
