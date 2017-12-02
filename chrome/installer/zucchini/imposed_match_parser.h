// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_IMPOSED_MATCH_PARSER_H_
#define CHROME_INSTALLER_ZUCCHINI_IMPOSED_MATCH_PARSER_H_

#include <stddef.h>

#include <ostream>
#include <string>
#include <vector>

#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/element_detection.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// Parses |imposed_matches| specified for |old_image| and |new_image|. If non-
// empty, |imposed_matches| should be formatted as:
//   "#+#=#+#,#+#=#+#,..."  (e.g., "1+2=3+4", "1+2=3+4,5+6=7+8"),
// where "#+#=#+#" encodes a match as 4 unsigned integers:
//   [offset in "old", size in "old", offset in "new", size in "new"].
// |detector| is used to validate Element types for matched pairs. |out_str|
// is used to emit warning and error messages. On success, writes results to
// the output parameters {|num_identical|, |matches|} (entries of |matches|
// are sorted by "new" position) and returns true. Otherwise returns false.
bool ParseImposedMatches(const std::string& imposed_matches,
                         ConstBufferView old_image,
                         ConstBufferView new_image,
                         ElementDetector&& detector,
                         std::ostream& out_str,
                         size_t* num_identical,
                         std::vector<ElementMatch>* matches);

// Encodes |match| as "#+#=#+#", where "#" denotes the integers:
//   [offset in "old", size in "old", offset in "new", size in "new"].
// Note that element type is omitted.
std::string FormatElementMatch(const ElementMatch& match);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_IMPOSED_MATCH_PARSER_H_
