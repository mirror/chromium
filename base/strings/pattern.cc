// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/pattern.h"

#include "base/third_party/icu/icu_utf.h"

namespace base {

namespace {

static bool IsWildcard(base_icu::UChar32 character) {
  return character == '*' || character == '?';
}

// Move the strings pointers to the point where they start to differ.
template <typename CHAR, typename NEXT>
bool SearchForChars(const CHAR** pattern,
                    const CHAR* pattern_end,
                    const CHAR** string,
                    const CHAR* string_end,
                    int maximum_distance,
                    NEXT next) {
  const CHAR* pattern_start = *pattern;
  const CHAR* string_start = *string;
  bool escape = false;
  while (true) {
    if (*pattern != pattern_end) {
      // If we have found a wildcard, we're done.
      if (!escape && IsWildcard(**pattern))
        break;

      // Check if the escape character is found. If so, skip it and move to the
      // next character.
      if (!escape && **pattern == '\\') {
        escape = true;
        next(pattern, pattern_end);
        continue;
      }

      escape = false;

      if (*string == string_end)
        return false;

      // Check if the chars match, if so, increment the ptrs.
      const CHAR* pattern_next = *pattern;
      const CHAR* string_next = *string;
      base_icu::UChar32 pattern_char = next(&pattern_next, pattern_end);
      if (pattern_char == next(&string_next, string_end) &&
          pattern_char != CBU_SENTINEL) {
        *pattern = pattern_next;
        *string = string_next;
        continue;
      }
    } else {
      if (*string == string_end)
        return true;
    }

    if (maximum_distance == 0)
      return false;

    // Restart at the beginning of the pattern with the next character in the
    // string.
    maximum_distance--;
    *pattern = pattern_start;
    next(&string_start, string_end);
    *string = string_start;
  }
  return true;
}

// Consumes consecutive wildcard characters (? or *). Returns the maximum number
// of characters matched by the sequence of wildcards, or -1 if the wildcards
// match an arbitrary number of characters (which is the case if it contains at
// least one *).
template <typename CHAR, typename NEXT>
int EatWildcards(const CHAR** pattern, const CHAR* end, NEXT next) {
  int num_question_marks = 0;
  bool has_asterisk = false;
  while (*pattern != end) {
    if (**pattern == '?') {
      num_question_marks++;
    } else if (**pattern == '*') {
      has_asterisk = true;
    } else {
      break;
    }

    next(pattern, end);
  }
  return has_asterisk ? -1 : num_question_marks;
}

template <typename CHAR, typename NEXT>
bool SearchForEnd(const CHAR** string,
                  const CHAR* string_end,
                  int maximum_distance,
                  NEXT next) {
  if (maximum_distance < 0)
    return true;

  while (*string != string_end) {
    if (maximum_distance == 0)
      return false;

    maximum_distance--;
    next(string, string_end);
  }
  return true;
}

template <typename CHAR, typename NEXT>
bool MatchPatternT(const CHAR* eval,
                   const CHAR* eval_end,
                   const CHAR* pattern,
                   const CHAR* pattern_end,
                   NEXT next) {
  while (true) {
    int maximum_wildcard_length = EatWildcards(&pattern, pattern_end, next);
    if (pattern == pattern_end)
      return SearchForEnd(&eval, eval_end, maximum_wildcard_length, next);

    if (!SearchForChars(&pattern, pattern_end, &eval, eval_end,
                        maximum_wildcard_length, next)) {
      break;
    }
  }
  return false;
}

struct NextCharUTF8 {
  base_icu::UChar32 operator()(const char** p, const char* end) {
    base_icu::UChar32 c;
    int offset = 0;
    CBU8_NEXT(*p, offset, end - *p, c);
    *p += offset;
    return c;
  }
};

struct NextCharUTF16 {
  base_icu::UChar32 operator()(const char16** p, const char16* end) {
    base_icu::UChar32 c;
    int offset = 0;
    CBU16_NEXT(*p, offset, end - *p, c);
    *p += offset;
    return c;
  }
};

}  // namespace

bool MatchPattern(StringPiece eval, StringPiece pattern) {
  return MatchPatternT(eval.data(), eval.data() + eval.size(), pattern.data(),
                       pattern.data() + pattern.size(), NextCharUTF8());
}

bool MatchPattern(StringPiece16 eval, StringPiece16 pattern) {
  return MatchPatternT(eval.data(), eval.data() + eval.size(), pattern.data(),
                       pattern.data() + pattern.size(), NextCharUTF16());
}

}  // namespace base
