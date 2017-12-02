// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_STRCAT_H_
#define BASE_STRINGS_STRCAT_H_

#include <initializer_list>

#include "base/base_export.h"
#include "base/compiler_specific.h"
#include "base/strings/string_piece.h"

namespace base {

// StrCat ----------------------------------------------------------------------
//
// StrCat is a function to perform concatenation on a sequence of strings.
// It is preferrable to a sequence of "a + b + c" because it is both faster and
// generates less code.
//
// MORE INFO
//
// StrCat can see all arguments at once, so it can allocate one return buffer
// of exactly the right size and copy once, as opposed to a sequence of
// operator+ which generates a series of temporary strings, copying as it goes.
// And by using StringPiece arguments, StrCat can avoid creating temporary
// string objects for char* constants.
//
// Prefer "+" to a the 2-argument version when dealing with string objects.
// StrCat has the same performance and generates the same or slightly more code
// than plain "+", while being more difficult to read. It makes sense only for
// concatenating two char* arguments, so only that specific variant is
// provided.
//
// 2-argument StrCat generates the same code for string + string because both
// take two string arguments and return a string, and generates slightly more
// code for char* + string (and the reverse) because operator+ provides an
// override for those exact arguments while StrCat forces the construction of a
// StringPiece.
//
// These functions have WARN_UNUSED_RESULT to catch people using StrCat
// like stdlib's version which appends to the first argument.
//
// We supply variants up to 10 strings. Above that, we can overflow to using
// initializer lists to support an infinite number of strings to append. Abseil
// does this after only 5 strings which makes the file much simpler. But our
// priority is code size and the initializer list variant is nontrivially
// larger than a simple function call.
//
// POSSIBLE ENHANCEMENTS
//
// Internal Google / Abseil has a similar StrCat function. If we convert to
// Abseil's StringView we can just use that implementation.
//
// Abseil's StrCat also allows numbers by using an intermediate class that can
// be implicitly constructed from either a string or various number types. This
// class formats the numbers into a static buffer for increased performance,
// and the call sites look nice.
//
// As-written Abseil's version generates slightly more code than the raw
// StringPiece version. We can de-inline the helper class' constructors which
// will cause the StringPiece constructors to be de-inlined for this call and
// generate slightly less code. This is something we can explore more in the
// future.

BASE_EXPORT std::string StrCat(const char* a,
                               const char* b) WARN_UNUSED_RESULT;  // See above.
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d,
                               StringPiece e) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d,
                               StringPiece e,
                               StringPiece f) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d,
                               StringPiece e,
                               StringPiece f,
                               StringPiece g) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d,
                               StringPiece e,
                               StringPiece f,
                               StringPiece g,
                               StringPiece h) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d,
                               StringPiece e,
                               StringPiece f,
                               StringPiece g,
                               StringPiece h,
                               StringPiece i) WARN_UNUSED_RESULT;
BASE_EXPORT std::string StrCat(StringPiece a,
                               StringPiece b,
                               StringPiece c,
                               StringPiece d,
                               StringPiece e,
                               StringPiece f,
                               StringPiece g,
                               StringPiece h,
                               StringPiece i,
                               StringPiece j) WARN_UNUSED_RESULT;

// 16-bit.
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d,
                            StringPiece16 e) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d,
                            StringPiece16 e,
                            StringPiece16 f) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d,
                            StringPiece16 e,
                            StringPiece16 f,
                            StringPiece16 g) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d,
                            StringPiece16 e,
                            StringPiece16 f,
                            StringPiece16 g,
                            StringPiece16 h) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d,
                            StringPiece16 e,
                            StringPiece16 f,
                            StringPiece16 g,
                            StringPiece16 h,
                            StringPiece16 i) WARN_UNUSED_RESULT;
BASE_EXPORT string16 StrCat(base::StringPiece16 a,
                            StringPiece16 b,
                            StringPiece16 c,
                            StringPiece16 d,
                            StringPiece16 e,
                            StringPiece16 f,
                            StringPiece16 g,
                            StringPiece16 h,
                            StringPiece16 i,
                            StringPiece16 j) WARN_UNUSED_RESULT;

// StrCat for >10 arguments ----------------------------------------------------
//
// Templatized version of StrCat that can accept any number of arguments.
//
// It converts the call to an initializer list and calls
// StrCatInitializerList(). Code can call this function directly if it has
// strings in an initializer list format. For normal uses the regular-argument
// version is preferred because it generates less code.

BASE_EXPORT std::string StrCatInitializerList(
    std::initializer_list<StringPiece> pieces);
BASE_EXPORT string16
StrCatInitializerList(std::initializer_list<StringPiece16> pieces);

template <typename... Pieces>
inline std::string StrCat(StringPiece a,
                          StringPiece b,
                          StringPiece c,
                          StringPiece d,
                          StringPiece e,
                          StringPiece f,
                          StringPiece g,
                          StringPiece h,
                          StringPiece i,
                          StringPiece j,
                          const Pieces&... args) {
  return StrCatInitializerList(
      {a, b, c, d, e, f, g, h, i, j, static_cast<const StringPiece&>(args)...});
}

template <typename... Pieces>
inline string16 StrCat(StringPiece16 a,
                       StringPiece16 b,
                       StringPiece16 c,
                       StringPiece16 d,
                       StringPiece16 e,
                       StringPiece16 f,
                       StringPiece16 g,
                       StringPiece16 h,
                       StringPiece16 i,
                       StringPiece16 j,
                       const Pieces&... args) {
  return StrCatInitializerList({a, b, c, d, e, f, g, h, i, j,
                                static_cast<const StringPiece16&>(args)...});
}

// StrAppend -------------------------------------------------------------------
//
// Appends a sequence of strings to a destination. Prefer:
//   StrAppend(&foo, ...);
// over:
//   foo += StrCat(...);
// because it avoids a temporary string allocation and copy.

BASE_EXPORT void StrAppend(std::string* dest, StringPiece a, StringPiece b);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d,
                           StringPiece e);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d,
                           StringPiece e,
                           StringPiece f);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d,
                           StringPiece e,
                           StringPiece f,
                           StringPiece g);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d,
                           StringPiece e,
                           StringPiece f,
                           StringPiece g,
                           StringPiece h);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d,
                           StringPiece e,
                           StringPiece f,
                           StringPiece g,
                           StringPiece h,
                           StringPiece i);
BASE_EXPORT void StrAppend(std::string* dest,
                           StringPiece a,
                           StringPiece b,
                           StringPiece c,
                           StringPiece d,
                           StringPiece e,
                           StringPiece f,
                           StringPiece g,
                           StringPiece h,
                           StringPiece i,
                           StringPiece j);
// 16-bit.
BASE_EXPORT void StrAppend(string16* dest, StringPiece16 a, StringPiece16 b);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d,
                           StringPiece16 e);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d,
                           StringPiece16 e,
                           StringPiece16 f);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d,
                           StringPiece16 e,
                           StringPiece16 f,
                           StringPiece16 g);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d,
                           StringPiece16 e,
                           StringPiece16 f,
                           StringPiece16 g,
                           StringPiece16 h);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d,
                           StringPiece16 e,
                           StringPiece16 f,
                           StringPiece16 g,
                           StringPiece16 h,
                           StringPiece16 i);
BASE_EXPORT void StrAppend(string16* dest,
                           StringPiece16 a,
                           StringPiece16 b,
                           StringPiece16 c,
                           StringPiece16 d,
                           StringPiece16 e,
                           StringPiece16 f,
                           StringPiece16 g,
                           StringPiece16 h,
                           StringPiece16 i,
                           StringPiece16 j);

// StrAppend for >10 arguments -------------------------------------------------
//
// See comments on StrCat for the initializer list versions.

BASE_EXPORT void StrAppendInitializerList(
    std::string* dest,
    std::initializer_list<StringPiece> pieces);
BASE_EXPORT void StrAppendInitializerList(
    string16* dest,
    std::initializer_list<StringPiece16> pieces);

template <typename... Pieces>
inline void StrAppend(std::string* dest,
                      StringPiece a,
                      StringPiece b,
                      StringPiece c,
                      StringPiece d,
                      StringPiece e,
                      StringPiece f,
                      StringPiece g,
                      StringPiece h,
                      StringPiece i,
                      StringPiece j,
                      const Pieces&... args) {
  StrAppendInitializerList(dest, {a, b, c, d, e, f, g, h, i, j,
                                  static_cast<const StringPiece&>(args)...});
}

template <typename... Pieces>
inline void StrAppend(string16* dest,
                      StringPiece16 a,
                      StringPiece16 b,
                      StringPiece16 c,
                      StringPiece16 d,
                      StringPiece16 e,
                      StringPiece16 f,
                      StringPiece16 g,
                      StringPiece16 h,
                      StringPiece16 i,
                      StringPiece16 j,
                      const Pieces&... args) {
  StrAppendInitializerList(dest, {a, b, c, d, e, f, g, h, i, j,
                                  static_cast<const StringPiece16&>(args)...});
}

}  // namespace base

#endif  // BASE_STRINGS_STRCAT_H_
