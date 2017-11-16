/*
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef SegmentedString_h
#define SegmentedString_h

#include "platform/PlatformExport.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Deque.h"
#include "platform/wtf/text/StringBuilder.h"
#include "platform/wtf/text/TextPosition.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

template <bool supports16bit>
class SegmentedSubstring {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  SegmentedSubstring() { data_.string8_ptr = nullptr; }

  SegmentedSubstring(const String& str) : length_(str.length()), string_(str) {
    if (length_) {
      if (supports16bit && !string_.Is8Bit()) {
        is8_bit_ = false;
        data_.string16_ptr = string_.Characters16();
        current_char_ = *data_.string16_ptr;
      } else {
        is8_bit_ = true;
        data_.string8_ptr = string_.Characters8();
        current_char_ = *data_.string8_ptr;
      }
    } else {
      is8_bit_ = true;
      data_.string8_ptr = nullptr;
    }
  }

  SegmentedSubstring(const SegmentedSubstring<!supports16bit>& s)
      : length_(s.length_),
        current_char_(s.current_char_),
        do_not_exclude_line_numbers_(s.do_not_exclude_line_numbers_),
        is8_bit_(s.is8_bit_),
        string_(s.string_) {
    data_.void_ptr = s.data_.void_ptr;
    if (!supports16bit) {
      DCHECK_EQ(true, is8_bit_);
      DCHECK_EQ(true, string_.Is8Bit());
    }
  }

  void Clear() {
    length_ = 0;
    data_.string8_ptr = nullptr;
    is8_bit_ = true;
    current_char_ = 0;
  }

  bool ExcludeLineNumbers() const { return !do_not_exclude_line_numbers_; }
  bool DoNotExcludeLineNumbers() const { return do_not_exclude_line_numbers_; }

  void SetExcludeLineNumbers() { do_not_exclude_line_numbers_ = false; }

  int NumberOfCharactersConsumed() const { return string_.length() - length_; }

  void AppendTo(StringBuilder& builder) const {
    int offset = string_.length() - length_;

    if (!offset) {
      if (length_)
        builder.Append(string_);
    } else {
      builder.Append(string_.Substring(offset, length_));
    }
  }

  bool PushIfPossible(UChar c) {
    if (!length_)
      return false;

    // This checks if either 8 or 16 bit strings are in the first character
    // (where we can't rewind). Since length_ is greater than zero, we can check
    // the Impl() directly and avoid a branch here.
    if (data_.void_ptr == string_.Impl()->Bytes())
      return false;

    if (supports16bit && !is8_bit_) {
      if (*(data_.string16_ptr - 1) != c)
        return false;

      --data_.string16_ptr;
    } else {
      if (*(data_.string8_ptr - 1) != c)
        return false;

      --data_.string8_ptr;
    }

    if (supports16bit)
      current_char_ = c;
    ++length_;
    return true;
  }

  UChar GetCurrentChar() const {
    return supports16bit ? current_char_ : *data_.string8_ptr;
  }

  void IncrementAndDecrementLength() {
    if (supports16bit) {
      if (is8_bit_)
        current_char_ = *++data_.string8_ptr;
      else
        current_char_ = *++data_.string16_ptr;
    } else {
      ++data_.string8_ptr;
    }
    --length_;
  }

  String CurrentSubString(unsigned length) {
    int offset = string_.length() - length_;
    return string_.Substring(offset, length);
  }

  ALWAYS_INLINE int length() const { return length_; }

 private:
  union {
    const LChar* string8_ptr;
    const UChar* string16_ptr;
    const void* void_ptr;
  } data_;
  int length_ = 0;
  UChar current_char_ = 0;
  bool do_not_exclude_line_numbers_ = true;
  bool is8_bit_ = true;
  String string_;

  friend class SegmentedSubstring<!supports16bit>;
};

template <bool supports16bit>
class SegmentedStringImpl {
  DISALLOW_NEW();

 public:
  SegmentedStringImpl()
      : number_of_characters_consumed_prior_to_current_string_(0),
        number_of_characters_consumed_prior_to_current_line_(0),
        current_line_(0),
        closed_(false),
        empty_(true) {}

  SegmentedStringImpl(const String& str)
      : current_string_(str),
        number_of_characters_consumed_prior_to_current_string_(0),
        number_of_characters_consumed_prior_to_current_line_(0),
        current_line_(0),
        closed_(false),
        empty_(!str.length()) {}

  SegmentedStringImpl(const SegmentedStringImpl<!supports16bit>& s)
      : current_string_(s.current_string_),
        number_of_characters_consumed_prior_to_current_string_(
            s.number_of_characters_consumed_prior_to_current_string_),
        number_of_characters_consumed_prior_to_current_line_(
            s.number_of_characters_consumed_prior_to_current_line_),
        current_line_(s.current_line_),
        closed_(s.closed_),
        empty_(s.empty_) {
    for (auto& substring : s.substrings_)
      substrings_.push_back(substring);
  }

  void Clear() {
    current_string_.Clear();
    number_of_characters_consumed_prior_to_current_string_ = 0;
    number_of_characters_consumed_prior_to_current_line_ = 0;
    current_line_ = 0;
    substrings_.clear();
    closed_ = false;
    empty_ = true;
  }

  void Close() {
    // Closing a stream twice is likely a coding mistake.
    DCHECK(!closed_);
    closed_ = true;
  }

  void Append(const SegmentedStringImpl& s) {
    DCHECK(!closed_);

    Append(s.current_string_);
    if (s.IsComposite()) {
      for (auto& substring : s.substrings_)
        Append(substring);
    }
  }

  enum class PrependType {
    kNewInput = 0,
    kUnconsume = 1,
  };
  void Prepend(const SegmentedStringImpl& s, PrependType type) {
    if (s.IsComposite()) {
      auto it = s.substrings_.rbegin();
      auto e = s.substrings_.rend();
      for (; it != e; ++it)
        Prepend(*it, type);
    }
    Prepend(s.current_string_, type);
  }

  bool ExcludeLineNumbers() const {
    return current_string_.ExcludeLineNumbers();
  }
  void SetExcludeLineNumbers() {
    current_string_.SetExcludeLineNumbers();
    if (IsComposite()) {
      for (auto& substring : substrings_)
        substring.SetExcludeLineNumbers();
    }
  }

  void Push(UChar c) {
    DCHECK(c);

    // pushIfPossible attempts to rewind the pointer in the SegmentedSubstring,
    // however it will fail if the SegmentedSubstring is empty, or
    // when we prepended some text while consuming a SegmentedSubstring by
    // document.write().
    if (current_string_.PushIfPossible(c))
      return;

    Prepend(SegmentedStringImpl(String(&c, 1)), PrependType::kUnconsume);
  }

  bool IsEmpty() const { return empty_; }

  unsigned length() const {
    unsigned length = current_string_.length();
    if (IsComposite()) {
      for (auto& substring : substrings_)
        length += substring.length();
    }
    return length;
  }

  bool IsClosed() const { return closed_; }

  enum LookAheadResult {
    kDidNotMatch,
    kDidMatch,
    kNotEnoughCharacters,
  };

  LookAheadResult LookAhead(const String& string) {
    return LookAheadInline(string, kTextCaseSensitive);
  }
  LookAheadResult LookAheadIgnoringCase(const String& string) {
    return LookAheadInline(string, kTextCaseASCIIInsensitive);
  }

  void Advance() {
    if (LIKELY(current_string_.length() > 1)) {
      current_string_.IncrementAndDecrementLength();
    } else {
      AdvanceSubstring();
    }
  }

  void UpdateLineNumber() {
    if (LIKELY(current_string_.DoNotExcludeLineNumbers())) {
      ++current_line_;
      // Plus 1 because numberOfCharactersConsumed value hasn't incremented yet;
      // it does with length() decrement below.
      number_of_characters_consumed_prior_to_current_line_ =
          NumberOfCharactersConsumed() + 1;
    }
  }

  void AdvanceAndUpdateLineNumber() {
    DCHECK_GE(current_string_.length(), 1);

    if (current_string_.GetCurrentChar() == '\n')
      UpdateLineNumber();

    if (LIKELY(current_string_.length() > 1)) {
      current_string_.IncrementAndDecrementLength();
    } else {
      AdvanceSubstring();
    }
  }

  void AdvanceAndASSERT(UChar expected_character) {
    DCHECK_EQ(expected_character, CurrentChar());
    Advance();
  }

  void AdvanceAndASSERTIgnoringCase(UChar expected_character) {
    DCHECK_EQ(WTF::Unicode::FoldCase(CurrentChar()),
              WTF::Unicode::FoldCase(expected_character));
    Advance();
  }

  void AdvancePastNonNewline() {
    DCHECK_NE(CurrentChar(), '\n');
    Advance();
  }

  void AdvancePastNewlineAndUpdateLineNumber() {
    DCHECK_EQ(CurrentChar(), '\n');
    DCHECK_GE(current_string_.length(), 1);

    UpdateLineNumber();

    if (LIKELY(current_string_.length() > 1)) {
      current_string_.IncrementAndDecrementLength();
    } else {
      AdvanceSubstring();
    }
  }

  // Writes the consumed characters into consumedCharacters, which must
  // have space for at least |count| characters.
  void Advance(unsigned count, UChar* consumed_characters) {
    SECURITY_DCHECK(count <= length());
    for (unsigned i = 0; i < count; ++i) {
      consumed_characters[i] = CurrentChar();
      Advance();
    }
  }

  int NumberOfCharactersConsumed() const {
    int number_of_pushed_characters = 0;
    return number_of_characters_consumed_prior_to_current_string_ +
           current_string_.NumberOfCharactersConsumed() -
           number_of_pushed_characters;
  }

  String ToString() const {
    StringBuilder result;
    current_string_.AppendTo(result);
    if (IsComposite()) {
      for (auto& substring : substrings_)
        substring.AppendTo(result);
    }
    return result.ToString();
  }

  UChar CurrentChar() const { return current_string_.GetCurrentChar(); }

  // The method is moderately slow, comparing to currentLine method.
  OrdinalNumber CurrentColumn() const {
    int zero_based_column =
        NumberOfCharactersConsumed() -
        number_of_characters_consumed_prior_to_current_line_;
    return OrdinalNumber::FromZeroBasedInt(zero_based_column);
  }

  OrdinalNumber CurrentLine() const {
    return OrdinalNumber::FromZeroBasedInt(current_line_);
  }

  // Sets value of line/column variables. Column is specified indirectly by a
  // parameter columnAftreProlog which is a value of column that we should get
  // after a prolog (first prologLength characters) has been consumed.
  void SetCurrentPosition(OrdinalNumber line,
                          OrdinalNumber column_aftre_prolog,
                          int prolog_length) {
    current_line_ = line.ZeroBasedInt();
    number_of_characters_consumed_prior_to_current_line_ =
        NumberOfCharactersConsumed() + prolog_length -
        column_aftre_prolog.ZeroBasedInt();
  }

 private:
  void Append(const SegmentedSubstring<supports16bit>& s) {
    DCHECK(!closed_);
    if (!s.length())
      return;

    if (!current_string_.length()) {
      number_of_characters_consumed_prior_to_current_string_ +=
          current_string_.NumberOfCharactersConsumed();
      current_string_ = s;
    } else {
      substrings_.push_back(s);
    }
    empty_ = false;
  }

  void Prepend(const SegmentedSubstring<supports16bit>& s, PrependType type) {
    DCHECK(!s.NumberOfCharactersConsumed());
    if (!s.length())
      return;

    // FIXME: We're also ASSERTing that s is a fresh SegmentedSubstring.
    //        The assumption is sufficient for our current use, but we might
    //        need to handle the more elaborate cases in the future.
    number_of_characters_consumed_prior_to_current_string_ +=
        current_string_.NumberOfCharactersConsumed();
    if (type == PrependType::kUnconsume)
      number_of_characters_consumed_prior_to_current_string_ -= s.length();
    if (!current_string_.length()) {
      current_string_ = s;
    } else {
      // Shift our m_currentString into our list.
      substrings_.push_front(current_string_);
      current_string_ = s;
    }
    empty_ = false;
  }

  void AdvanceSubstring() {
    if (IsComposite()) {
      number_of_characters_consumed_prior_to_current_string_ +=
          current_string_.NumberOfCharactersConsumed() + 1;
      current_string_ = substrings_.TakeFirst();
      // If we've previously consumed some characters of the non-current
      // string, we now account for those characters as part of the current
      // string, not as part of "prior to current string."
      number_of_characters_consumed_prior_to_current_string_ -=
          current_string_.NumberOfCharactersConsumed();
    } else {
      current_string_.Clear();
      empty_ = true;
    }
  }

  inline LookAheadResult LookAheadInline(const String& string,
                                         TextCaseSensitivity case_sensitivity) {
    if (string.length() <= static_cast<unsigned>(current_string_.length())) {
      String current_substring =
          current_string_.CurrentSubString(string.length());
      if (current_substring.StartsWith(string, case_sensitivity))
        return kDidMatch;
      return kDidNotMatch;
    }
    return LookAheadSlowCase(string, case_sensitivity);
  }

  LookAheadResult LookAheadSlowCase(const String& string,
                                    TextCaseSensitivity case_sensitivity) {
    unsigned count = string.length();
    if (count > length())
      return kNotEnoughCharacters;
    UChar* consumed_characters;
    String consumed_string =
        String::CreateUninitialized(count, consumed_characters);
    Advance(count, consumed_characters);
    LookAheadResult result = kDidNotMatch;
    if (consumed_string.StartsWith(string, case_sensitivity))
      result = kDidMatch;
    Prepend(SegmentedStringImpl(consumed_string), PrependType::kUnconsume);
    return result;
  }

  bool IsComposite() const { return !substrings_.IsEmpty(); }

  SegmentedSubstring<supports16bit> current_string_;
  int number_of_characters_consumed_prior_to_current_string_;
  int number_of_characters_consumed_prior_to_current_line_;
  int current_line_;
  Deque<SegmentedSubstring<supports16bit>> substrings_;
  bool closed_;
  bool empty_;

  friend class SegmentedStringImpl<!supports16bit>;
};

using SegmentedString = SegmentedStringImpl<true>;

}  // namespace blink

#endif
