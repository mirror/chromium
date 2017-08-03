// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/in_memory_url_index_types.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <set>

#include "base/i18n/break_iterator.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string_util.h"
#include "net/base/escape.h"

#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"

namespace {
// The maximum number of characters to consider from an URL and page title
// while matching user-typed terms.
const size_t kMaxSignificantChars = 200;

// Adapter class for base::i18n::BreakIterator to handle underscore as a
// punctuation.
// |sub_words_| holds words which are split by underscore if current word has
// underscore.
// |pos_| indicates position of |sub_words_| considering underscore.
class BreakIteratorAdapter {
 public:
  explicit BreakIteratorAdapter(base::i18n::BreakIterator& iter)
      : iter_(iter) {}
  bool Init() { return iter_.Init(); }
  bool IsWord() const { return iter_.IsWord(); }

  bool Advance() {
    if (AdvanceSubWords())
      return true;

    if (!iter_.Advance())
      return false;

    BreakSubWords();
    return true;
  }

  base::string16 GetString() const {
    if (!sub_words_.empty())
      return sub_words_.back();
    return iter_.GetString();
  }

  size_t prev() const {
    if (!sub_words_.empty())
      return pos_;
    return iter_.prev();
  }

 private:
  bool AdvanceSubWords() {
    if (sub_words_.size() > 1) {
      const auto& word = sub_words_.back();
      // Adjust position by word length + underscore.
      pos_ += word.length() + 1;
      sub_words_.pop_back();
      return true;
    }
    if (!sub_words_.empty())
      sub_words_.clear();
    return false;
  }
  void BreakSubWords() {
    if (!iter_.IsWord())
      return;
    base::string16 word = iter_.GetString();
    std::size_t found = word.find(underscore_);
    if (found == std::string::npos)
      return;
    sub_words_ = base::SplitString(word, underscore_, base::TRIM_WHITESPACE,
                                   base::SPLIT_WANT_NONEMPTY);
    std::reverse(sub_words_.begin(),sub_words_.end());
    pos_ = iter_.prev();
  }
  const base::string16 underscore_ = base::ASCIIToUTF16("_");
  std::vector<base::string16> sub_words_;
  size_t pos_ = 0;
  base::i18n::BreakIterator& iter_;
  DISALLOW_COPY_AND_ASSIGN(BreakIteratorAdapter);
};
}

// Matches within URL and Title Strings ----------------------------------------

TermMatches MatchTermInString(const base::string16& term,
                              const base::string16& cleaned_string,
                              int term_num) {
  const size_t kMaxCompareLength = 2048;
  const base::string16& short_string =
      (cleaned_string.length() > kMaxCompareLength) ?
      cleaned_string.substr(0, kMaxCompareLength) : cleaned_string;
  TermMatches matches;
  for (size_t location = short_string.find(term);
       location != base::string16::npos;
       location = short_string.find(term, location + 1))
    matches.push_back(TermMatch(term_num, location, term.length()));
  return matches;
}

// Comparison function for sorting TermMatches by their offsets.
bool MatchOffsetLess(const TermMatch& m1, const TermMatch& m2) {
  return m1.offset < m2.offset;
}

TermMatches SortMatches(const TermMatches& matches) {
  TermMatches sorted_matches(matches);
  std::sort(sorted_matches.begin(), sorted_matches.end(), MatchOffsetLess);
  return sorted_matches;
}

// Assumes |sorted_matches| is already sorted.
TermMatches DeoverlapMatches(const TermMatches& sorted_matches) {
  TermMatches out;
  std::copy_if(
      sorted_matches.begin(), sorted_matches.end(), std::back_inserter(out),
      [&out](const TermMatch& match) {
        return out.empty() ||
               match.offset >= (out.back().offset + out.back().length); });
  return out;
}

std::vector<size_t> OffsetsFromTermMatches(const TermMatches& matches) {
  std::vector<size_t> offsets;
  for (const auto& match : matches) {
    offsets.push_back(match.offset);
    offsets.push_back(match.offset + match.length);
  }
  return offsets;
}

TermMatches ReplaceOffsetsInTermMatches(const TermMatches& matches,
                                        const std::vector<size_t>& offsets) {
  DCHECK_EQ(2 * matches.size(), offsets.size());
  TermMatches new_matches;
  std::vector<size_t>::const_iterator offset_iter = offsets.begin();
  for (TermMatches::const_iterator term_iter = matches.begin();
       term_iter != matches.end(); ++term_iter, ++offset_iter) {
    const size_t starting_offset = *offset_iter;
    ++offset_iter;
    const size_t ending_offset = *offset_iter;
    if ((starting_offset != base::string16::npos) &&
        (ending_offset != base::string16::npos) &&
        (starting_offset != ending_offset)) {
      TermMatch new_match(*term_iter);
      new_match.offset = starting_offset;
      new_match.length = ending_offset - starting_offset;
      new_matches.push_back(new_match);
    }
  }
  return new_matches;
}

// Utility Functions -----------------------------------------------------------

String16Set String16SetFromString16(const base::string16& cleaned_uni_string,
                                    WordStarts* word_starts) {
  String16Vector words =
      String16VectorFromString16(cleaned_uni_string, false, word_starts);
  for (auto& word : words)
    word = base::i18n::ToLower(word).substr(0, kMaxSignificantChars);
  return String16Set(std::make_move_iterator(words.begin()),
                     std::make_move_iterator(words.end()),
                     base::KEEP_FIRST_OF_DUPES);
}

String16Vector String16VectorFromString16(
    const base::string16& cleaned_uni_string,
    bool break_on_space,
    WordStarts* word_starts) {
  if (word_starts)
    word_starts->clear();
  base::i18n::BreakIterator break_iter(cleaned_uni_string,
      break_on_space ? base::i18n::BreakIterator::BREAK_SPACE :
          base::i18n::BreakIterator::BREAK_WORD);
  BreakIteratorAdapter iter(break_iter);
  String16Vector words;
  if (!iter.Init())
    return words;
  while (iter.Advance()) {
    if (break_on_space || iter.IsWord()) {
      base::string16 word(iter.GetString());
      size_t initial_whitespace = 0;
      if (break_on_space) {
        base::string16 trimmed_word;
        base::TrimWhitespace(word, base::TRIM_LEADING, &trimmed_word);
        initial_whitespace = word.length() - trimmed_word.length();
        base::TrimWhitespace(trimmed_word, base::TRIM_TRAILING, &word);
      }
      if (word.empty())
        continue;
      words.push_back(word);
      if (!word_starts)
        continue;
      size_t word_start = iter.prev() + initial_whitespace;
      if (word_start < kMaxSignificantChars)
        word_starts->push_back(word_start);
    }
  }
  return words;
}

Char16Set Char16SetFromString16(const base::string16& term) {
  return Char16Set(term.begin(), term.end(), base::KEEP_FIRST_OF_DUPES);
}

// HistoryInfoMapValue ---------------------------------------------------------

HistoryInfoMapValue::HistoryInfoMapValue() = default;
HistoryInfoMapValue::HistoryInfoMapValue(const HistoryInfoMapValue& other) =
    default;
HistoryInfoMapValue::HistoryInfoMapValue(HistoryInfoMapValue&& other) = default;
HistoryInfoMapValue& HistoryInfoMapValue::operator=(
    const HistoryInfoMapValue& other) = default;
HistoryInfoMapValue& HistoryInfoMapValue::operator=(
    HistoryInfoMapValue&& other) = default;
HistoryInfoMapValue::~HistoryInfoMapValue() = default;

// RowWordStarts ---------------------------------------------------------------

RowWordStarts::RowWordStarts() = default;
RowWordStarts::RowWordStarts(const RowWordStarts& other) = default;
RowWordStarts::RowWordStarts(RowWordStarts&& other) = default;
RowWordStarts& RowWordStarts::operator=(const RowWordStarts& other) = default;
RowWordStarts& RowWordStarts::operator=(RowWordStarts&& other) = default;
RowWordStarts::~RowWordStarts() = default;

void RowWordStarts::Clear() {
  url_word_starts_.clear();
  title_word_starts_.clear();
}
