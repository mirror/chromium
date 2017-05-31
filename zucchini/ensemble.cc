// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/ensemble.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

#include "zucchini/binary_data_histogram.h"
#include "zucchini/image_utils.h"
#include "zucchini/io_utils.h"
#include "zucchini/program_detector.h"
#include "zucchini/ranges/utility.h"

namespace zucchini {

namespace {

// Determines whether a proposed comparison between Elements should be rejected
// early, to decrease the likelyhood of creating false-positive matches, which
// may be costly for patching. Our heuristic simply prohibits big difference in
// size (relative and absolute) between matched elements.
bool UnsafeDifference(const Ensemble::Element& old_element,
                      const Ensemble::Element& new_element) {
  static constexpr double kMaxBloat = 2.0;
  static constexpr size_t kMinWorrysomeDifference = 2 << 20;  // 2MB
  size_t old_size = old_element.sub_image.size();
  size_t new_size = new_element.sub_image.size();
  size_t lo_size = std::min(old_size, new_size);
  size_t hi_size = std::max(old_size, new_size);
  if (hi_size - lo_size < kMinWorrysomeDifference)
    return false;
  if (hi_size < lo_size * kMaxBloat)
    return false;
  return true;
}

std::ostream& operator<<(std::ostream& stream, const Ensemble::Element& elt) {
  size_t pos = elt.start_offset();
  size_t size = elt.sub_image.size();
  stream << "(" << elt.exe_type << ", ";
  stream << AsHex<8, size_t>(pos) << " +" << AsHex<8, size_t>(size) << ")";
  return stream;
}

bool IsMatchDEX(const EnsembleMatcher::Match& match) {
  return match.old_element.exe_type == EXE_TYPE_DEX;
}

// Modifies |matches| to trim away unfavorable ones.
void TrimMatches(std::vector<EnsembleMatcher::Match>* matches) {
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

Ensemble::Ensemble(Region image) : image_(image) {}

Ensemble::~Ensemble() = default;

size_t Ensemble::FindEmbeddedElements(SingleProgramDetector* detector) {
  elements_.clear();
  ProgramDetector program_detector(image_);
  while (program_detector.GetNext(detector)) {
    if (elements_.size() >= kElementLimit)
      return kElementLimit;
    elements_.push_back({detector->exe_type(), image_, detector->region()});
  }
  return elements_.size();
}

/******** EnsembleMatcher ********/

EnsembleMatcher::EnsembleMatcher() = default;

// static
bool EnsembleMatcher::ParseImposedMatches(
    const std::string& imposed_matches,
    Region old_image,
    Region new_image,
    SingleProgramDetector* detector,
    std::ostream& out_str,
    size_t* num_identical,
    std::vector<EnsembleMatcher::Match>* matches) {
  size_t temp_num_identical = 0;
  std::vector<Match> temp_matches;

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
    temp_matches.push_back({{EXE_TYPE_UNKNOWN,  // Assign later.
                             old_image,
                             {old_image.begin() + old_offset, old_size}},
                            {EXE_TYPE_UNKNOWN,  // Assign later.
                             new_image,
                             {new_image.begin() + new_offset, new_size}}});
  }
  // Sort matches by "new" file offsets. This helps with overlap checks.
  std::sort(temp_matches.begin(), temp_matches.end(),
            [](const Match& match_a, const Match& match_b) {
              return match_a.new_element.start_offset() <
                     match_b.new_element.start_offset();
            });

  // Check for overlaps in "new" file.
  for (size_t i = 1; i < temp_matches.size(); ++i) {
    if (temp_matches[i - 1].new_element.end_offset() >
        temp_matches[i].new_element.start_offset()) {
      out_str << "Imposed matches have overlap in \"new\" file." << std::endl;
      return false;
    }
  }

  // Compute types and verify consistency. Remove identical matches and matches
  // where any sub-image has an unknown type.
  size_t write_idx = 0;
  for (size_t read_idx = 0; read_idx < temp_matches.size(); ++read_idx) {
    Region old_sub_image = temp_matches[read_idx].old_element.sub_image;
    Region new_sub_image = temp_matches[read_idx].new_element.sub_image;
    // Remove identical match.
    if (ranges::equal(old_sub_image, new_sub_image)) {
      ++temp_num_identical;
      continue;
    }
    // Check executable types of sub-images.
    ExecutableType old_exe_type =
        detector->Run(old_sub_image) ? detector->exe_type() : EXE_TYPE_UNKNOWN;
    ExecutableType new_exe_type =
        detector->Run(new_sub_image) ? detector->exe_type() : EXE_TYPE_UNKNOWN;

    if (old_exe_type == EXE_TYPE_UNKNOWN || new_exe_type == EXE_TYPE_UNKNOWN) {
      // Skip unknown types, including those mixed with known types.
      out_str << "Warning: Skipping unknown type in match: "
              << temp_matches[read_idx] << "." << std::endl;
      continue;
    } else if (old_exe_type != new_exe_type) {
      // Inconsistant known types are errors.
      out_str << "Inconsistent types in match: " << temp_matches[read_idx]
              << "." << std::endl;
      return false;
    }

    // Keep match and remove gaps.
    temp_matches[read_idx].old_element.exe_type = old_exe_type;
    temp_matches[read_idx].new_element.exe_type = new_exe_type;
    if (write_idx < read_idx)
      temp_matches[write_idx] = temp_matches[read_idx];
    ++write_idx;
  }
  temp_matches.resize(write_idx);

  *num_identical = temp_num_identical;
  matches->swap(temp_matches);
  return true;
}

bool EnsembleMatcher::RunMatch(Region old_image,
                               Region new_image,
                               const std::string& imposed_matches) {
  Reset();
  DisassemblerSingleProgramDetector detector;
  if (imposed_matches.empty()) {
    // No imposed matches: Apply heuristics to detect and match.
    auto find_embedded = [&detector](const char* name, Ensemble* ensemble) {
      size_t num_elts = ensemble->FindEmbeddedElements(&detector);
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
    if (!ParseImposedMatches(imposed_matches, old_image, new_image, &detector,
                             std::cout, &num_identical_, &matches_)) {
      return false;
    }
  }
  TrimMatches(&matches_);
  GenerateSeparators(new_image);
  return true;
}

bool EnsembleMatcher::RunRawMatch(Region old_image, Region new_image) {
  Reset();
  matches_.push_back({{EXE_TYPE_NO_OP, old_image, old_image},
                      {EXE_TYPE_NO_OP, new_image, new_image}});
  return true;
}

void EnsembleMatcher::Reset() {
  matches_.clear();
  separators_.clear();
  num_identical_ = 0;
}

void EnsembleMatcher::GenerateSeparators(Region new_image) {
  Region::iterator it = new_image.begin();
  for (Match& match : matches_) {
    Region new_sub_image = match.new_element.sub_image;
    separators_.push_back(Region(it, new_sub_image.begin()));
    it = new_sub_image.end();
  }
  separators_.push_back(Region(it, new_image.end()));
}

void EnsembleMatcher::MatchEnsembles(const Ensemble& old_ensemble,
                                     const Ensemble& new_ensemble) {
  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);
  Reset();

  const Ensemble::ElementList& old_elements = old_ensemble.GetElements();
  const Ensemble::ElementList& new_elements = new_ensemble.GetElements();

  if (verbose_) {
    std::cout << "Comparing old (" << old_elements.size() << " elements) and ";
    std::cout << "new (" << new_elements.size() << " elements)" << std::endl;
  }

  std::vector<BinaryDataHistogram> old_his(old_elements.size());
  for (size_t iold = 0; iold < old_elements.size(); ++iold) {
    Region sub_image = old_elements[iold].sub_image;
    old_his[iold].Compute(sub_image);
    // ProgramDetector should have imposed minimal size limit to |sub_image|.
    // Therefore we expect all resulting histogram to be valid.
    CHECK(old_his[iold].IsValid());
  }

  // Text grid-representation of matches, for verbose output.
  std::vector<std::string> summary_lines;

  for (size_t inew = 0; inew < new_elements.size(); ++inew) {
    const Ensemble::Element& cur_new_element = new_elements[inew];
    Region cur_new_sub_image = cur_new_element.sub_image;
    BinaryDataHistogram new_his;
    new_his.Compute(cur_new_sub_image);
    CHECK(new_his.IsValid());

    std::string summary_line;
    double best_dist = HUGE_VAL;
    size_t best_iold = old_elements.size();
    bool is_identical = false;

    for (size_t iold = 0; iold < old_elements.size(); ++iold) {
      const Ensemble::Element& cur_old_element = old_elements[iold];
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
      double dist = old_his[iold].Compare(new_his);
      if (verbose_) {
        std::cout << "Compare old" << cur_old_element;
        std::cout << " to new" << cur_new_element;
        std::cout << " --> " << std::fixed << std::setprecision(2) << dist;
        std::cout << std::endl;
        summary_line += 'C';  // 'C' for "Comparison candidate".
      }

      if (best_dist > dist) {
        best_iold = iold;
        best_dist = dist;
        if (best_dist == 0 &&
            ranges::equal(cur_old_element.sub_image, cur_new_sub_image)) {
          is_identical = true;
          break;
        }
      }
    }

    if (best_iold < old_elements.size()) {
      const Ensemble::Element& best_old_element = old_elements[best_iold];

      if (verbose_) {
        // 'I' for "Identical"; 'M' for "Matched".
        summary_line[best_iold] = is_identical ? 'I' : 'M';
      }

      // Identical matches are excluded from |matches_|.
      if (is_identical)
        ++num_identical_;
      else
        matches_.push_back({best_old_element, cur_new_element});

      if (verbose_) {
        if (is_identical) {
          std::cout << "Skipped old" << best_old_element;
          std::cout << " - identical to new" << cur_new_element;
        } else {
          std::cout << "Matched old" << best_old_element;
          std::cout << " to new" << cur_new_element;
          std::cout << " --> " << std::fixed << std::setprecision(2)
                    << best_dist;
        }
        std::cout << std::endl;
      }
    }
    if (verbose_)
      summary_lines.push_back(summary_line);
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

std::ostream& operator<<(std::ostream& ostr,
                         const EnsembleMatcher::Match& match) {
  ostr << match.old_element.start_offset() << "+"
       << match.old_element.sub_image.size() << "="
       << match.new_element.start_offset() << "+"
       << match.new_element.sub_image.size();
  return ostr;
}

}  // namespace zucchini
