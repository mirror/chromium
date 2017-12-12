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

#include "platform/text/SegmentedString.h"

namespace blink {

template <bool supports16bit>
unsigned SegmentedStringImpl<supports16bit>::length() const {
  unsigned length = current_string_.length();
  if (IsComposite()) {
    for (auto& substring : substrings_)
      length += substring.length();
  }
  return length;
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::SetExcludeLineNumbers() {
  current_string_.SetExcludeLineNumbers();
  if (IsComposite()) {
    for (auto& substring : substrings_)
      substring.SetExcludeLineNumbers();
  }
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Clear() {
  current_string_.Clear();
  number_of_characters_consumed_prior_to_current_string_ = 0;
  number_of_characters_consumed_prior_to_current_line_ = 0;
  current_line_ = 0;
  substrings_.clear();
  closed_ = false;
  empty_ = true;
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Append(
    const SegmentedSubstring<supports16bit>& s) {
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

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Push(UChar c) {
  DCHECK(c);
  DCHECK(supports16bit || !(c & 0xff00));

  // pushIfPossible attempts to rewind the pointer in the SegmentedSubstring,
  // however it will fail if the SegmentedSubstring is empty, or
  // when we prepended some text while consuming a SegmentedSubstring by
  // document.write().
  if (current_string_.PushIfPossible(c))
    return;

  std::conditional_t<supports16bit, UChar, LChar> arg = c;
  Prepend(SegmentedString(String(&arg, 1)), PrependType::kUnconsume);
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Prepend(
    const SegmentedSubstring<supports16bit>& s,
    PrependType type) {
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

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Close() {
  // Closing a stream twice is likely a coding mistake.
  DCHECK(!closed_);
  closed_ = true;
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Append(
    const SegmentedStringImpl<supports16bit>& s) {
  DCHECK(!closed_);

  Append(s.current_string_);
  if (s.IsComposite()) {
    for (auto& substring : s.substrings_)
      Append(substring);
  }
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Prepend(
    const SegmentedStringImpl<supports16bit>& s,
    PrependType type) {
  if (s.IsComposite()) {
    auto it = s.substrings_.rbegin();
    auto e = s.substrings_.rend();
    for (; it != e; ++it)
      Prepend(*it, type);
  }
  Prepend(s.current_string_, type);
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::AdvanceSubstring() {
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

template <bool supports16bit>
String SegmentedStringImpl<supports16bit>::ToString() const {
  StringBuilder result;
  current_string_.AppendTo(result);
  if (IsComposite()) {
    for (auto& substring : substrings_)
      substring.AppendTo(result);
  }
  return result.ToString();
}

template <>
void SegmentedStringImpl<false>::Advance(unsigned count,
                                         LChar* consumed_characters) {
  SECURITY_DCHECK(count <= length());
  for (unsigned i = 0; i < count; ++i) {
    consumed_characters[i] = CurrentChar();
    Advance();
  }
}

template <>
void SegmentedStringImpl<true>::Advance(unsigned count,
                                        LChar* consumed_characters) {
  NOTREACHED();
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::Advance(unsigned count,
                                                 UChar* consumed_characters) {
  SECURITY_DCHECK(count <= length());
  for (unsigned i = 0; i < count; ++i) {
    consumed_characters[i] = CurrentChar();
    Advance();
  }
}

template <bool supports16bit>
OrdinalNumber SegmentedStringImpl<supports16bit>::CurrentLine() const {
  return OrdinalNumber::FromZeroBasedInt(current_line_);
}

template <bool supports16bit>
OrdinalNumber SegmentedStringImpl<supports16bit>::CurrentColumn() const {
  int zero_based_column = NumberOfCharactersConsumed() -
                          number_of_characters_consumed_prior_to_current_line_;
  return OrdinalNumber::FromZeroBasedInt(zero_based_column);
}

template <bool supports16bit>
void SegmentedStringImpl<supports16bit>::SetCurrentPosition(
    OrdinalNumber line,
    OrdinalNumber column_aftre_prolog,
    int prolog_length) {
  current_line_ = line.ZeroBasedInt();
  number_of_characters_consumed_prior_to_current_line_ =
      NumberOfCharactersConsumed() + prolog_length -
      column_aftre_prolog.ZeroBasedInt();
}

template class SegmentedStringImpl<true>;
template class SegmentedStringImpl<false>;

}  // namespace blink
