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
  SegmentedSubstring() { void_ptr = nullptr; }

  SegmentedSubstring(const String& str) : length_(str.length()), string_(str) {
    if (length_) {
      if (supports16bit && !string_.Is8Bit()) {
        is8_bit_ = false;
        string16_ptr = string_.Characters16();
        current_char_ = *string16_ptr;
      } else {
        is8_bit_ = true;
        string8_ptr = string_.Characters8();
        current_char_ = *string8_ptr;
      }
    } else {
      is8_bit_ = true;
      string8_ptr = nullptr;
    }
  }

  SegmentedSubstring(const SegmentedSubstring<!supports16bit>& s)
      : length_(s.length_),
        current_char_(s.current_char_),
        do_not_exclude_line_numbers_(s.do_not_exclude_line_numbers_),
        is8_bit_(s.is8_bit_),
        string_(s.string_) {
    void_ptr = s.void_ptr;
    if (!supports16bit) {
      DCHECK_EQ(true, is8_bit_);
      DCHECK_EQ(true, string_.Is8Bit());
    }
  }

  void Clear() {
    length_ = 0;
    void_ptr = nullptr;
    if (supports16bit) {
      is8_bit_ = true;
      current_char_ = 0;
    }
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
    if (void_ptr == string_.Impl()->Bytes())
      return false;

    if (supports16bit && !is8_bit_) {
      if (*(string16_ptr - 1) != c)
        return false;

      --string16_ptr;
    } else {
      if (*(string8_ptr - 1) != c)
        return false;

      --string8_ptr;
    }

    if (supports16bit)
      current_char_ = c;
    ++length_;
    return true;
  }

  UChar GetCurrentChar() const {
    return supports16bit ? current_char_ : *string8_ptr;
  }

  void IncrementAndDecrementLength() {
    if (supports16bit) {
      if (is8_bit_)
        current_char_ = *++string8_ptr;
      else
        current_char_ = *++string16_ptr;
    } else {
      ++string8_ptr;
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
  };
  int length_ = 0;
  UChar current_char_ = 0;
  bool do_not_exclude_line_numbers_ = true;
  bool is8_bit_ = true;
  String string_;

  friend class SegmentedSubstring<!supports16bit>;
};

template <bool supports16bit>
class PLATFORM_EXPORT SegmentedStringImpl {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

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

  void Clear();
  void Close();

  void Append(const SegmentedStringImpl&);
  enum class PrependType {
    kNewInput = 0,
    kUnconsume = 1,
  };
  void Prepend(const SegmentedStringImpl&, PrependType);

  bool ExcludeLineNumbers() const {
    return current_string_.ExcludeLineNumbers();
  }
  void SetExcludeLineNumbers();

  void Push(UChar);

  bool IsEmpty() const { return empty_; }
  unsigned length() const;

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

  ALWAYS_INLINE void Advance() {
    if (LIKELY(current_string_.length() > 1)) {
      current_string_.IncrementAndDecrementLength();
    } else {
      AdvanceSubstring();
    }
  }

  ALWAYS_INLINE void UpdateLineNumber() {
    if (LIKELY(current_string_.DoNotExcludeLineNumbers())) {
      ++current_line_;
      // Plus 1 because numberOfCharactersConsumed value hasn't incremented yet;
      // it does with length() decrement below.
      number_of_characters_consumed_prior_to_current_line_ =
          NumberOfCharactersConsumed() + 1;
    }
  }

  ALWAYS_INLINE void AdvanceAndUpdateLineNumber() {
    DCHECK_GE(current_string_.length(), 1);

    if (current_string_.GetCurrentChar() == '\n')
      UpdateLineNumber();

    if (LIKELY(current_string_.length() > 1)) {
      current_string_.IncrementAndDecrementLength();
    } else {
      AdvanceSubstring();
    }
  }

  ALWAYS_INLINE void AdvanceAndASSERT(UChar expected_character) {
    DCHECK_EQ(expected_character, CurrentChar());
    Advance();
  }

  ALWAYS_INLINE void AdvanceAndASSERTIgnoringCase(UChar expected_character) {
    DCHECK_EQ(WTF::Unicode::FoldCase(CurrentChar()),
              WTF::Unicode::FoldCase(expected_character));
    Advance();
  }

  ALWAYS_INLINE void AdvancePastNonNewline() {
    DCHECK_NE(CurrentChar(), '\n');
    Advance();
  }

  ALWAYS_INLINE void AdvancePastNewlineAndUpdateLineNumber() {
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
  void Advance(unsigned count, UChar* consumed_characters);

  int NumberOfCharactersConsumed() const {
    int number_of_pushed_characters = 0;
    return number_of_characters_consumed_prior_to_current_string_ +
           current_string_.NumberOfCharactersConsumed() -
           number_of_pushed_characters;
  }

  String ToString() const;

  ALWAYS_INLINE UChar CurrentChar() const {
    return current_string_.GetCurrentChar();
  }

  // The method is moderately slow, comparing to currentLine method.
  OrdinalNumber CurrentColumn() const;
  OrdinalNumber CurrentLine() const;
  // Sets value of line/column variables. Column is specified indirectly by a
  // parameter columnAftreProlog which is a value of column that we should get
  // after a prolog (first prologLength characters) has been consumed.
  void SetCurrentPosition(OrdinalNumber line,
                          OrdinalNumber column_aftre_prolog,
                          int prolog_length);

 private:
  void Append(const SegmentedSubstring<supports16bit>&);
  void Prepend(const SegmentedSubstring<supports16bit>&, PrependType);

  void AdvanceSubstring();

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
