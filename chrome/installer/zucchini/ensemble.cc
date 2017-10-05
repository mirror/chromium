// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/ensemble.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#include "base/bind.h"
#include "chrome/installer/zucchini/binary_data_histogram.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

namespace {

// Takes a sequence of values, computes basic statistics, and heuristically
// decides whether query values are outliers.
class OutlierDetector {
 public:
  // Includes |v| into existing statistics.
  void Add(double v) {
    ++n_;
    sum_ += v;
    sum_of_squares_ += v * v;
  }

  // Prepares basic statistics for IsOutlier() calls.
  void Prepare() {
    if (n_ > 0) {
      mean_ = sum_ / n_;
      standard_deviation_ = ::sqrt((sum_of_squares_ - sum_ * mean_) / n_);
    }
  }

  void PrintStats() {
    std::cout << "Mean = " << mean_ << ", "
              << "StdDev = " << standard_deviation_;
  }

  // Heuristically decide whether |v| is an outlier.
  bool IsOutlier(double v) {
    // Ad hoc constants selected for our use.
    // Prevents divide-by-zero and avoids penalizing tight clusters.
    constexpr double kMinTolerance = 0.1;
    constexpr double kSigmaBound = 2.5;
    if (n_ <= 2)
      return false;
    double tolerance = std::min(kMinTolerance, standard_deviation_);
    double sigmas = fabs(v - mean_) / tolerance;
    return sigmas > kSigmaBound;
  }

 private:
  size_t n_ = 0;
  double sum_ = 0;
  double sum_of_squares_ = 0;
  double mean_ = 0;
  double standard_deviation_ = 0;
};

// Determines whether a proposed comparison between Elements should be rejected
// early, to decrease the likelihood of creating false-positive matches, which
// may be costly for patching. Our heuristic simply prohibits big difference in
// size (relative and absolute) between matched elements.
bool UnsafeDifference(const Element& old_element, const Element& new_element) {
  static constexpr double kMaxBloat = 2.0;
  static constexpr size_t kMinWorrysomeDifference = 2 << 20;  // 2MB
  size_t old_size = old_element.length;
  size_t new_size = new_element.length;
  size_t lo_size = std::min(old_size, new_size);
  size_t hi_size = std::max(old_size, new_size);
  if (hi_size - lo_size < kMinWorrysomeDifference)
    return false;
  if (hi_size < lo_size * kMaxBloat)
    return false;
  return true;
}

std::ostream& operator<<(std::ostream& stream, const Element& elt) {
  size_t pos = elt.offset;
  size_t size = elt.length;
  stream << "(" << elt.exe_type << ", ";
  stream << AsHex<8, size_t>(pos) << " +" << AsHex<8, size_t>(size) << ")";
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
    std::cout << "Found " << dex_count << " DEX: Ignoring all." << std::endl;
    matches->erase(std::remove_if(matches->begin(), matches->end(), IsMatchDEX),
                   matches->end());
  }
}

}  // namespace

/******** Ensemble ********/

Ensemble::Ensemble(ConstBufferView image) : image_(image) {}

Ensemble::~Ensemble() = default;

size_t Ensemble::FindEmbeddedElements(ElementDetector&& detector) {
  elements_.clear();
  ElementFinder element_finder(image_, std::move(detector));
  for (auto element = element_finder.GetNext();
       element && elements_.size() < kElementLimit;
       element = element_finder.GetNext()) {
    elements_.push_back(*element);
  }
  return elements_.size();
}

/******** EnsembleMatcher ********/

EnsembleMatcher::EnsembleMatcher() = default;

// static
bool EnsembleMatcher::ParseImposedMatches(const std::string& imposed_matches,
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
    offset_t old_offset;
    size_t old_size;
    offset_t new_offset;
    size_t new_size;
    if (!(iss >> StrictUInt<offset_t>(old_offset) >> EatChar('+') >>
          StrictUInt<size_t>(old_size) >> EatChar('=') >>
          StrictUInt<offset_t>(new_offset) >> EatChar('+') >>
          StrictUInt<size_t>(new_size))) {
      out_str << "Parse error in imposed matches." << std::endl;
      return false;
    }
    // Check bounds.
    if (old_size == 0 || new_size == 0 ||
        !CheckDataFit(old_offset, old_size, old_image.size()) ||
        !CheckDataFit(new_offset, new_size, new_image.size())) {
      out_str << "Imposed matches have out-of-bound entry." << std::endl;
      return false;
    }
    temp_matches.push_back({{kExeTypeUnknown,  // Assign later.
                             offset_t(old_offset), offset_t(old_size)},
                            {kExeTypeUnknown,  // Assign later.
                             offset_t(new_offset), offset_t(new_size)}});
  }
  // Sort matches by "new" file offsets. This helps with overlap checks.
  std::sort(temp_matches.begin(), temp_matches.end(),
            [](const ElementMatch& match_a, const ElementMatch& match_b) {
              return match_a.new_element.offset < match_b.new_element.offset;
            });

  // Check for overlaps in "new" file.
  for (size_t i = 1; i < temp_matches.size(); ++i) {
    if (temp_matches[i - 1].new_element.offset +
            temp_matches[i - 1].new_element.length >
        temp_matches[i].new_element.offset) {
      out_str << "Imposed matches have overlap in \"new\" file." << std::endl;
      return false;
    }
  }

  // Compute types and verify consistency. Remove identical matches and matches
  // where any sub-image has an unknown type.
  size_t write_idx = 0;
  for (size_t read_idx = 0; read_idx < temp_matches.size(); ++read_idx) {
    ConstBufferView old_sub_image(
        old_image.begin() + temp_matches[read_idx].old_element.offset,
        temp_matches[read_idx].old_element.length);
    ConstBufferView new_sub_image(
        new_image.begin() + temp_matches[read_idx].new_element.offset,
        temp_matches[read_idx].new_element.length);
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
              << temp_matches[read_idx] << "." << std::endl;
      continue;
    } else if (old_element->exe_type != new_element->exe_type) {
      // Inconsistent known types are errors.
      out_str << "Inconsistent types in match: " << temp_matches[read_idx]
              << "." << std::endl;
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

bool EnsembleMatcher::RunMatch(ConstBufferView old_image,
                               ConstBufferView new_image,
                               const std::string& imposed_matches) {
  Reset();
  if (imposed_matches.empty()) {
    // No imposed matches: Apply heuristics to detect and match.
    auto find_embedded = [](const char* name, Ensemble* ensemble) {
      size_t num_elts = ensemble->FindEmbeddedElements(
          base::Bind(DetectElementFromDisassembler));
      if (num_elts >= Ensemble::kElementLimit) {
        std::cout << name << ": Error: Found too many elements." << std::endl;
        return false;
      }
      std::cout << name << ": Found " << num_elts << " elements." << std::endl;
      return true;
    };
    Ensemble old_ensemble(old_image);
    Ensemble new_ensemble(new_image);
    if (!find_embedded("Old file", &old_ensemble) ||
        !find_embedded("New file", &new_ensemble)) {
      return false;
    }
    MatchEnsembles(old_ensemble, new_ensemble);
  } else {
    if (!ParseImposedMatches(imposed_matches, old_image, new_image,
                             base::Bind(DetectElementFromDisassembler),
                             std::cout, &num_identical_, &matches_)) {
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
  matches_.push_back({{kExeTypeNoOp, offset_t(0), offset_t(old_image.size())},
                      {kExeTypeNoOp, offset_t(0), offset_t(new_image.size())}});
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
    ConstBufferView new_sub_image(new_image.begin() + match.new_element.offset,
                                  match.new_element.length);
    separators_.push_back(
        ConstBufferView::FromRange(it, new_sub_image.begin()));
    it = new_sub_image.end();
  }
  separators_.push_back(ConstBufferView::FromRange(it, new_image.end()));
}

void EnsembleMatcher::MatchEnsembles(const Ensemble& old_ensemble,
                                     const Ensemble& new_ensemble) {
  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);
  Reset();

  const std::vector<Element>& old_elements = old_ensemble.GetElements();
  const std::vector<Element>& new_elements = new_ensemble.GetElements();

  if (verbose_) {
    std::cout << "Comparing old (" << old_elements.size() << " elements) and ";
    std::cout << "new (" << new_elements.size() << " elements)" << std::endl;
  }

  std::vector<BinaryDataHistogram> old_his(old_elements.size());
  for (size_t iold = 0; iold < old_elements.size(); ++iold) {
    ConstBufferView sub_image(
        old_ensemble.image().begin() + old_elements[iold].offset,
        old_elements[iold].length);
    old_his[iold].Compute(sub_image);
    // ProgramDetector should have imposed minimal size limit to |sub_image|.
    // Therefore we expect all resulting histogram to be valid.
    CHECK(old_his[iold].IsValid());
  }

  // Text grid-representation of matches, for verbose output.
  std::vector<std::string> summary_lines;

  struct Results {
    int iold;
    int inew;
    double score;
  };
  std::vector<Results> results;

  int num_new_elements = static_cast<int>(new_elements.size());
  int num_old_elements = static_cast<int>(old_elements.size());

  for (int inew = 0; inew < num_new_elements; ++inew) {
    const Element& cur_new_element = new_elements[inew];
    ConstBufferView cur_new_sub_image(
        new_ensemble.image().begin() + cur_new_element.offset,
        cur_new_element.length);
    BinaryDataHistogram new_his;
    new_his.Compute(cur_new_sub_image);
    CHECK(new_his.IsValid());

    std::string summary_line;
    double best_dist = HUGE_VAL;
    int best_iold = num_old_elements;
    bool is_identical = false;

    for (int iold = 0; iold < num_old_elements; ++iold) {
      const Element& cur_old_element = old_elements[iold];
      if (cur_old_element.exe_type != cur_new_element.exe_type) {
        if (verbose_)
          summary_line += 'T';  // 'T' for "Type mismatch".
        continue;
      }
      if (UnsafeDifference(cur_old_element, cur_new_element)) {
        if (verbose_)
          summary_line += 'U';  // 'U' for "Unsafe difference".
        continue;
      }
      double dist = old_his[iold].Distance(new_his);
      if (verbose_) {
        std::cout << "Compare old" << cur_old_element;
        std::cout << " to new" << cur_new_element;
        std::cout << " --> " << std::fixed << std::setprecision(5) << dist;
        std::cout << std::endl;
        summary_line += 'C';  // 'C' for "Comparison candidate".
      }

      if (best_dist > dist) {
        best_iold = iold;
        best_dist = dist;
        ConstBufferView sub_image(
            old_ensemble.image().begin() + cur_old_element.offset,
            cur_old_element.length);
        if (best_dist == 0 && sub_image.equals(cur_new_sub_image)) {
          is_identical = true;
          break;
        }
      }
    }

    if (best_iold < num_old_elements) {
      const Element& best_old_element = old_elements[best_iold];

      if (verbose_) {
        // 'I' for "Identical"; 'M' for "Matched".
        summary_line[best_iold] = is_identical ? 'I' : 'M';
      }

      // Identical matches are excluded from |matches_|.
      if (is_identical)
        ++num_identical_;
      else
        results.push_back({best_iold, inew, best_dist});

      if (verbose_) {
        if (is_identical) {
          std::cout << "Skipped old" << best_old_element;
          std::cout << " - identical to new" << cur_new_element;
        } else {
          std::cout << "Matched old" << best_old_element;
          std::cout << " to new" << cur_new_element;
          std::cout << " --> " << std::fixed << std::setprecision(5)
                    << best_dist;
        }
        std::cout << std::endl;
      }
    }
    if (verbose_)
      summary_lines.push_back(summary_line);
  }

  // Heuristic to reject poor matches: Gather statistics for score. If a score
  // looks like an outlier then reject the associated match.
  if (results.size() > 0) {
    OutlierDetector detector;
    for (const auto& result : results)
      detector.Add(result.score);
    detector.Prepare();
    std::cout << "Best scores: ";
    detector.PrintStats();
    std::cout << std::endl;
    for (const Results& result : results) {
      if (detector.IsOutlier(result.score)) {
        if (verbose_)
          summary_lines[result.inew][result.iold] = 'O';  // 'O' for "Outlier".
      } else {
        matches_.push_back(
            {old_elements[result.iold], new_elements[result.inew]});
      }
    }
  }

  if (verbose_) {
    for (const std::string& line : summary_lines) {
      for (char ch : line) {
        char prefix = (ch == 'I' || ch == 'M') ? '(' : ' ';
        char suffix = (ch == 'I' || ch == 'M') ? ')' : ' ';
        std::cout << prefix << ch << suffix;
      }
      std::cout << std::endl;
    }
  }

  std::cout.copyfmt(old_fmt);
}

std::ostream& operator<<(std::ostream& ostr, const ElementMatch& match) {
  ostr << match.old_element.offset << "+" << match.old_element.length << "="
       << match.new_element.offset << "+" << match.new_element.length;
  return ostr;
}

}  // namespace zucchini
