// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/ensemble.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "chrome/installer/zucchini/binary_data_histogram.h"
#include "chrome/installer/zucchini/imposed_match_parser.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

namespace {

/******** Helper Functions ********/

// Determines whether a proposed comparison between Elements should be rejected
// early, to decrease the likelihood of creating false-positive matches, which
// may be costly for patching. Our heuristic simply prohibits big difference in
// size (relative and absolute) between matched elements.
bool UnsafeDifference(const Element& old_element, const Element& new_element) {
  static constexpr double kMaxBloat = 2.0;
  static constexpr size_t kMinWorrysomeDifference = 2 << 20;  // 2MB
  size_t lo_size = std::min(old_element.size, new_element.size);
  size_t hi_size = std::max(old_element.size, new_element.size);
  if (hi_size - lo_size < kMinWorrysomeDifference)
    return false;
  if (hi_size < lo_size * kMaxBloat)
    return false;
  return true;
}

std::ostream& operator<<(std::ostream& stream, const Element& elt) {
  stream << "(" << elt.exe_type << ", " << AsHex<8, size_t>(elt.offset) << " +"
         << AsHex<8, size_t>(elt.size) << ")";
  return stream;
}

bool IsMatchDEX(const ElementMatch& match) {
  return match.old_element.exe_type == kExeTypeDex;
}

// Modifies |matches| to trim away unfavorable ones.
void TrimMatches(std::vector<ElementMatch>* matches) {
  // Trim rule: If > 1 DEX files are found then ignore all DEX. This is done
  // because we do not yet support MultiDex, under which contents can move
  // across file boundary between "old" and "new" archives. So forcing matches
  // of DEX files and patching them separately can create barriers that lead to
  // larger patches than naive patching.
  auto dex_count = std::count_if(matches->begin(), matches->end(), IsMatchDEX);
  if (dex_count > 1) {
    LOG(WARNING) << "Found " << dex_count << " DEX: Ignoring all." << std::endl;
    matches->erase(std::remove_if(matches->begin(), matches->end(), IsMatchDEX),
                   matches->end());
  }
}

/******** MatchingInfoOut ********/

// A class to output detailed information on ensemble matching. This decouples
// formatting and printing logic from matching logic.
class MatchingInfoOut {
 public:
  // If |out| is null then only basic information is printed using LOG().
  // Otherwise detailed information is printed using |out|, including comparison
  // pairs, scores, and a text grid representation of pairwise matching results.
  explicit MatchingInfoOut(std::ostream* out) : out_(out) {}
  ~MatchingInfoOut() = default;

  // Outputs sizes and initializes |text_grid_|.
  void InitSizes(size_t old_size, size_t new_size) {
    if (out_) {
      (*out_) << "Comparing old (" << old_size << " elements) and ";
      (*out_) << "new (" << new_size << " elements)" << std::endl;
    }
    // If |out_| is null then |text_grid_| won't get printed. But to keep code
    // short we initialize it and update it anyway.
    text_grid_.resize(new_size, std::string(old_size, '-'));
    best_dist_.resize(new_size, -1.0);
  }

  // Functions to update match status in text grid representation.

  void DeclareTypeMismatch(int iold, int inew) { text_grid_[inew][iold] = 'T'; }
  void DeclareUnsafeDistance(int iold, int inew) {
    text_grid_[inew][iold] = 'U';
  }
  void DeclareCandidate(int iold, int inew) {
    text_grid_[inew][iold] = 'C';  // Provisional.
  }
  void DeclareMatch(int iold, int inew, double dist, bool is_identical) {
    text_grid_[inew][iold] = is_identical ? 'I' : 'M';
    best_dist_[inew] = dist;
  }
  void DeclareOutlier(int iold, int inew) { text_grid_[inew][iold] = 'O'; }

  // Functions to print detailed information.

  void OutputCompare(const Element& old_element,
                     const Element& new_element,
                     double dist) {
    if (out_) {
      (*out_) << "Compare old" << old_element << " to new" << new_element
              << " --> " << base::StringPrintf("%.5f", dist) << std::endl;
    }
  }

  void OutputMatch(const Element& best_old_element,
                   const Element& new_element,
                   bool is_identical,
                   double best_dist) {
    if (!out_)
      return;
    if (is_identical) {
      (*out_) << "Skipped old" << best_old_element << " - identical to new"
              << new_element;
    } else {
      (*out_) << "Matched old" << best_old_element << " to new" << new_element
              << " --> " << base::StringPrintf("%.5f", best_dist);
    }
    (*out_) << std::endl;
  }

  void OutputScores(const std::string& stats) {
    if (out_)
      (*out_) << "Best dists: " << stats << std::endl;
    else
      LOG(INFO) << "Best dists: " << stats;
  }

  void OutputTextGrid() {
    if (!out_)
      return;
    int new_size = static_cast<int>(text_grid_.size());
    for (int inew = 0; inew < new_size; ++inew) {
      const std::string& line = text_grid_[inew];
      (*out_) << "  ";
      for (char ch : line) {
        char prefix = (ch == 'I' || ch == 'M') ? '(' : ' ';
        char suffix = (ch == 'I' || ch == 'M') ? ')' : ' ';
        (*out_) << prefix << ch << suffix;
      }
      if (best_dist_[inew] >= 0)
        (*out_) << "   " << base::StringPrintf("%.5f", best_dist_[inew]);
      (*out_) << std::endl;
    }
    if (!text_grid_.empty()) {
      (*out_) << "  Legend: I = identical, M = matched, T = type mismatch, "
                 "U = unsafe distance, C = candidate, O = outlier, - = skipped."
              << std::endl;
    }
  }

 private:
  std::ostream* out_;

  // Text grid representation of matches. Rows correspond to "old" and columns
  // correspond to "new".
  std::vector<std::string> text_grid_;

  // For each "new" element, distance of best match. -1 denotes no match.
  std::vector<double> best_dist_;
};

}  // namespace

/******** Ensemble ********/

Ensemble::Ensemble(ConstBufferView image, const std::string& name)
    : image_(image), name_(name) {}

Ensemble::~Ensemble() = default;

bool Ensemble::FindEmbeddedElements(ElementDetector&& detector) {
  elements_.clear();
  ElementFinder element_finder(image_, std::move(detector));
  for (auto element = element_finder.GetNext();
       element.has_value() && elements_.size() <= kElementLimit;
       element = element_finder.GetNext()) {
    elements_.push_back(*element);
  }
  if (elements_.size() >= kElementLimit) {
    LOG(WARNING) << name_ << ": Found too many elements.";
    return false;
  }
  LOG(INFO) << name_ << ": Found " << elements_.size() << " elements.";
  return true;
}

/******** EnsembleMatcher ********/

EnsembleMatcher::EnsembleMatcher(std::ostream* out) : out_(out) {}

bool EnsembleMatcher::RunMatch(ConstBufferView old_image,
                               ConstBufferView new_image,
                               const std::string& imposed_matches) {
  LOG(INFO) << "Start matching.";
  Reset();
  if (imposed_matches.empty()) {
    // No imposed matches: Apply heuristics to detect and match.
    auto find_embedded = [](Ensemble* ensemble) {
      return ensemble->FindEmbeddedElements(
          base::Bind(DetectElementFromDisassembler));
    };
    Ensemble old_ensemble(old_image, "Old file");
    Ensemble new_ensemble(new_image, "New file");
    if (!find_embedded(&old_ensemble) || !find_embedded(&new_ensemble))
      return false;
    MatchEnsembles(old_ensemble, new_ensemble);
  } else {
    if (!ParseImposedMatches(imposed_matches, old_image, new_image,
                             base::Bind(DetectElementFromDisassembler),
                             std::cout, &num_identical_, &matches_)) {
      LOG(WARNING) << "Invalid imposed match.";
      return false;
    }
  }
  TrimMatches(&matches_);
  GenerateSeparators(new_image);
  return true;
}

bool EnsembleMatcher::RunRawMatch(ConstBufferView old_image,
                                  ConstBufferView new_image) {
  Reset();
  matches_.push_back({{{0, old_image.size()}, kExeTypeNoOp},
                      {{0, new_image.size()}, kExeTypeNoOp}});
  return true;
}

void EnsembleMatcher::Reset() {
  matches_.clear();
  separators_.clear();
  num_identical_ = 0;
}

void EnsembleMatcher::GenerateSeparators(ConstBufferView new_image) {
  ConstBufferView::iterator it = new_image.begin();
  for (ElementMatch& match : matches_) {
    ConstBufferView new_sub_image(new_image[match.new_element.region()]);
    separators_.push_back(
        ConstBufferView::FromRange(it, new_sub_image.begin()));
    it = new_sub_image.end();
  }
  separators_.push_back(ConstBufferView::FromRange(it, new_image.end()));
}

void EnsembleMatcher::MatchEnsembles(const Ensemble& old_ensemble,
                                     const Ensemble& new_ensemble) {
  DCHECK(matches_.empty());
  DCHECK(separators_.empty());
  MatchingInfoOut info_out(out_);

  const std::vector<Element>& new_elements = new_ensemble.GetElements();
  const std::vector<Element>& old_elements = old_ensemble.GetElements();
  const int num_new_elements = base::checked_cast<int>(new_elements.size());
  const int num_old_elements = base::checked_cast<int>(old_elements.size());
  info_out.InitSizes(num_old_elements, num_new_elements);

  // For each "new" element, match it with the "old" element that's nearest to
  // it, with distance determined by BinaryDataHistogram. The resulting
  // "old"-"new" pairs are stored into |results|. Possibilities:
  // - Type mismatch: No match.
  // - UnsafeDifference() heuristics fail: No match.
  // - Identical match: Skip "new" since this is a trivial case.
  // - Non-identical match: Match "new" with "old" with min distance.
  // - No match: Skip "new".
  struct Results {
    int iold;
    int inew;
    double dist;
  };
  std::vector<Results> results;

  // Precompute histograms for "old" since they get reused.
  std::vector<BinaryDataHistogram> old_his(num_old_elements);
  for (int iold = 0; iold < num_old_elements; ++iold) {
    ConstBufferView sub_image(old_ensemble.image()[old_elements[iold]]);
    old_his[iold].Compute(sub_image);
    // ProgramDetector should have imposed minimal size limit to |sub_image|.
    // Therefore resulting histogram are expected to be valid.
    CHECK(old_his[iold].IsValid());
  }

  const int kUninitIold = num_old_elements;
  for (int inew = 0; inew < num_new_elements; ++inew) {
    const Element& cur_new_element = new_elements[inew];
    ConstBufferView cur_new_sub_image(
        new_ensemble.image()[cur_new_element.region()]);
    BinaryDataHistogram new_his;
    new_his.Compute(cur_new_sub_image);
    CHECK(new_his.IsValid());

    double best_dist = HUGE_VAL;
    int best_iold = kUninitIold;
    bool is_identical = false;

    for (int iold = 0; iold < num_old_elements; ++iold) {
      const Element& cur_old_element = old_elements[iold];
      if (cur_old_element.exe_type != cur_new_element.exe_type) {
        info_out.DeclareTypeMismatch(iold, inew);
        continue;
      }
      if (UnsafeDifference(cur_old_element, cur_new_element)) {
        info_out.DeclareUnsafeDistance(iold, inew);
        continue;
      }
      double dist = old_his[iold].Distance(new_his);
      info_out.DeclareCandidate(iold, inew);
      info_out.OutputCompare(cur_old_element, cur_new_element, dist);
      if (best_dist > dist) {  // Tie resolution: First-one, first-serve.
        best_iold = iold;
        best_dist = dist;
        if (best_dist == 0) {
          ConstBufferView sub_image(
              old_ensemble.image()[cur_old_element.region()]);
          if (sub_image.equals(cur_new_sub_image)) {
            is_identical = true;
            break;
          }
        }
      }
    }

    if (best_iold != kUninitIold) {
      const Element& best_old_element = old_elements[best_iold];
      info_out.DeclareMatch(best_iold, inew, best_dist, is_identical);
      if (is_identical)  // Skip "new" if identical match is found.
        ++num_identical_;
      else
        results.push_back({best_iold, inew, best_dist});
      info_out.OutputMatch(best_old_element, cur_new_element, is_identical,
                           best_dist);
    }
  }

  // Populate |matches_| from |result|. To reduce chance of false-positive
  // matches, statistics on dists are computed. If a match's |dist| is an
  // outlier then it is rejected.
  if (results.size() > 0) {
    OutlierDetector detector;
    for (const auto& result : results) {
      if (result.dist > 0)
        detector.Add(result.dist);
    }
    detector.Prepare();
    info_out.OutputScores(detector.RenderStats());
    for (const Results& result : results) {
      if (detector.DecideOutlier(result.dist) > 0) {
        info_out.DeclareOutlier(result.iold, result.inew);
      } else {
        matches_.push_back(
            {old_elements[result.iold], new_elements[result.inew]});
      }
    }
    info_out.OutputTextGrid();
  }
}

}  // namespace zucchini
