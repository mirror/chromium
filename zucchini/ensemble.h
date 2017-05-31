// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_ENSEMBLE_H_
#define ZUCCHINI_ENSEMBLE_H_

#include <iostream>
#include <string>
#include <vector>

#include "base/macros.h"

#include "zucchini/disassembler.h"
#include "zucchini/region.h"

namespace zucchini {

class SingleProgramDetector;

// A collection of exectuables found in an image.
class Ensemble {
 public:
  // Maximum number of Elements in a file. This is enforced because our matching
  // algorithm is O(n^2), which suffices for regular archive files that should
  // have up to 10's of executable files. An archive containing 100's of
  // executables seems pathological; we disallow this for security.
  static constexpr size_t kElementLimit = 256;

  // An exectuable embedded in an image. Copy by value is allowed.
  struct Element {
    offset_t start_offset() const {
      return static_cast<offset_t>(sub_image.begin() - base_image.begin());
    }
    offset_t end_offset() const {
      return static_cast<offset_t>(sub_image.end() - base_image.begin());
    }
    ExecutableType exe_type;
    Region base_image;
    Region sub_image;
  };
  using ElementList = std::vector<Element>;

  explicit Ensemble(Region image);
  ~Ensemble();

  Region image() const { return image_; }

  // Uses |detector| to find embedded executables inside |image_|, and stores
  // the results into |elements_|. Returns the number of elements found, capped
  // at |kElementLimit| -- if there are too many element then we give up, and
  // this should be treated as an error.
  size_t FindEmbeddedElements(SingleProgramDetector* detector);

  const std::vector<Element>& GetElements() const { return elements_; }

 private:
  Region image_;
  ElementList elements_;

  DISALLOW_COPY_AND_ASSIGN(Ensemble);
};

// A class to find Elements in a "new" file and match each of these to an
// Element in an "old" file. A matched pair of Elements must have the same
// ExecutableType. Matches may be externally imposed, or heuristically generated
// by this class. For the latter case, a matched pair should also:
// - Have "reasonable" size difference (see UnsafeDifference() in the .cc file).
// - Have "minimal distance" among other potential matched pairs.
// Notes:
// - Special case: Exact matches are ignored, since they're trivial to patch.
// - Multiple "new" Elements may match a common "old" Element.
// - A "new" Element may have no match. This can happen when we skip an exact
//   match, or when no viable match exist.
class EnsembleMatcher {
 public:
  // A matched pair of Elements.
  struct Match {
    Ensemble::Element old_element;
    Ensemble::Element new_element;
  };

  EnsembleMatcher();

  // Parses |imposed_matches| specified for |old_image| and |new_image|. If non-
  // empty, |imposed_matches| should be formatted as:
  //   "#+#=#+#,#+#=#+#,..."  (e.g., "1+2=3+4", "1+2=3+4,5+6=7+8"),
  // where "#+#=#+#" encodes a match as 4 unsigned integers:
  //   [offset in "old", size in "old", offset in "new", size in "new"].
  // |detector| is used to validate Element types for matched pairs. |out_str|
  // is used to emit warning and error messages. On success, writes results to
  // the output parameters {|num_identical|, |matches|} (entries of |matches|
  // are sorted by "new" position) and returns true. Otherwise returns false.
  static bool ParseImposedMatches(const std::string& imposed_matches,
                                  Region old_image,
                                  Region new_image,
                                  SingleProgramDetector* detector,
                                  std::ostream& out_str,
                                  size_t* num_identical,
                                  std::vector<Match>* matches);

  // Enables verbose output, for debugging (e.g., Zucchini-match).
  void SetVerbose(bool verbose) { verbose_ = verbose; }

  // Finds elements in |old_image| (non-unique) that match elements in
  // |new_image| (unique), using |imposed_matches|. If |imposed_matches| is
  // empty, then detects programs in |old_image| and |new_image|, then calls
  // MatchEnsembles(). Otherwise calls ParseImposedMatches(). Returns true on
  // success.
  bool RunMatch(Region old_image,
                Region new_image,
                const std::string& imposed_matches);

  // An alternative to RunMatch() that trivially matches |old_image| to
  // |new_image| in entirety as a raw patch.
  bool RunRawMatch(Region old_image, Region new_image);

  // Return RunMatch() or RunRawMatch() results.
  const std::vector<Match>& GetMatches() const { return matches_; }
  const std::vector<Region>& GetSeparators() const { return separators_; }

  // Returns the number of identical matches found.
  size_t GetNumIdentical() const { return num_identical_; }

 private:
  void Reset();

  // Populates |separators_| from |new_image_| and |new_ensemble|.
  void GenerateSeparators(Region new_image);

  // Executes main algorithm to match |old_ensemble| with |new_ensemble|.
  // Results are stored as states in this class.
  void MatchEnsembles(const Ensemble& old_ensemble,
                      const Ensemble& new_ensemble);

  // Storage of matches elements: A list of matched pairs with increasing and
  // non-overlapping |new_element| entries.
  std::vector<Match> matches_;

  // Storage of regions before / between (successive) / after |matches_|
  // elements in |new_ensemble|, including empty regions. Contains 1 more
  // element than |matches_|.
  std::vector<Region> separators_;

  // Number of identical matches.
  size_t num_identical_ = 0;

  // Controls the quantity of information printed to console.
  bool verbose_ = false;
};

std::ostream& operator<<(std::ostream& ostr,
                         const EnsembleMatcher::Match& match);

}  // namespace zucchini

#endif  // ZUCCHINI_ENSEMBLE_H_
