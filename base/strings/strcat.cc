// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/strcat.h"

namespace base {

namespace {

// Reserves an additional amount of size in the given string, growing by at
// least 2x if the affected string is nonempty. Used by StrAppend().
//
// The "at least 2x" growing rule duplicates the exponential growth of
// std::string. The problem is that most implementations of reserve() will grow
// exactly to the requested amount instead of exponentially growing like would
// happen when appending normally. If we didn't do this, an append after the
// call to StrAppend() would definitely cause a reallocation, and loops with
// StrAppend() calls would have O(n^2) complexity to execute.
//
// If the string is empty, we assume that exponential growth is not necessary.
template <typename String>
void ReserveAdditional(String* str, typename String::size_type additional) {
  if (str->empty())
    str->reserve(additional);
  else
    str->reserve(std::max(str->size() + additional, str->size() * 2));
}

}  // namespace

// StrCat 8-bit ----------------------------------------------------------------

std::string StrCat(const char* a, const char* b) {
  size_t a_len = strlen(a);
  size_t b_len = strlen(b);

  std::string result;
  result.reserve(a_len + b_len);
  result.append(a, a_len);
  result.append(b, b_len);
  return result;
}

std::string StrCat(StringPiece a, StringPiece b, StringPiece c) {
  std::string result;
  StrAppend(&result, a, b, c);
  return result;
}

std::string StrCat(StringPiece a, StringPiece b, StringPiece c, StringPiece d) {
  std::string result;
  StrAppend(&result, a, b, c, d);
  return result;
}

std::string StrCat(StringPiece a,
                   StringPiece b,
                   StringPiece c,
                   StringPiece d,
                   StringPiece e) {
  std::string result;
  StrAppend(&result, a, b, c, d, e);
  return result;
}

std::string StrCat(StringPiece a,
                   StringPiece b,
                   StringPiece c,
                   StringPiece d,
                   StringPiece e,
                   StringPiece f) {
  std::string result;
  StrAppend(&result, a, b, c, d, e, f);
  return result;
}

std::string StrCat(StringPiece a,
                   StringPiece b,
                   StringPiece c,
                   StringPiece d,
                   StringPiece e,
                   StringPiece f,
                   StringPiece g) {
  std::string result;
  StrAppend(&result, a, b, c, d, e, f, g);
  return result;
}

std::string StrCat(StringPiece a,
                   StringPiece b,
                   StringPiece c,
                   StringPiece d,
                   StringPiece e,
                   StringPiece f,
                   StringPiece g,
                   StringPiece h) {
  std::string result;
  StrAppend(&result, a, b, c, d, e, f, g, h);
  return result;
}

std::string StrCat(StringPiece a,
                   StringPiece b,
                   StringPiece c,
                   StringPiece d,
                   StringPiece e,
                   StringPiece f,
                   StringPiece g,
                   StringPiece h,
                   StringPiece i) {
  std::string result;
  StrAppend(&result, a, b, c, d, e, f, g, h, i);
  return result;
}

std::string StrCat(StringPiece a,
                   StringPiece b,
                   StringPiece c,
                   StringPiece d,
                   StringPiece e,
                   StringPiece f,
                   StringPiece g,
                   StringPiece h,
                   StringPiece i,
                   StringPiece j) {
  std::string result;
  StrAppend(&result, a, b, c, d, e, f, g, h, i, j);
  return result;
}

// StrCat 16-bit ---------------------------------------------------------------

string16 StrCat(StringPiece16 a, StringPiece16 b, StringPiece16 c) {
  string16 result;
  StrAppend(&result, a, b, c);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d) {
  string16 result;
  StrAppend(&result, a, b, c, d);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d,
                StringPiece16 e) {
  string16 result;
  StrAppend(&result, a, b, c, d, e);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d,
                StringPiece16 e,
                StringPiece16 f) {
  string16 result;
  StrAppend(&result, a, b, c, d, e, f);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d,
                StringPiece16 e,
                StringPiece16 f,
                StringPiece16 g) {
  string16 result;
  StrAppend(&result, a, b, c, d, e, f, g);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d,
                StringPiece16 e,
                StringPiece16 f,
                StringPiece16 g,
                StringPiece16 h) {
  string16 result;
  StrAppend(&result, a, b, c, d, e, f, g, h);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d,
                StringPiece16 e,
                StringPiece16 f,
                StringPiece16 g,
                StringPiece16 h,
                StringPiece16 i) {
  string16 result;
  StrAppend(&result, a, b, c, d, e, f, g, h, i);
  return result;
}

string16 StrCat(StringPiece16 a,
                StringPiece16 b,
                StringPiece16 c,
                StringPiece16 d,
                StringPiece16 e,
                StringPiece16 f,
                StringPiece16 g,
                StringPiece16 h,
                StringPiece16 i,
                StringPiece16 j) {
  string16 result;
  StrAppend(&result, a, b, c, d, e, f, g, h, i, j);
  return result;
}

// StrCatInitializerList -------------------------------------------------------

std::string StrCatInitializerList(std::initializer_list<StringPiece> pieces) {
  std::string result;
  StrAppendInitializerList(&result, pieces);
  return result;
}

string16 StrCatInitializerList(std::initializer_list<StringPiece16> pieces) {
  string16 result;
  StrAppendInitializerList(&result, pieces);
  return result;
}

// StrAppend 8-bit -------------------------------------------------------------

void StrAppend(std::string* dest, StringPiece a, StringPiece b) {
  ReserveAdditional(dest, a.size() + b.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
}

void StrAppend(std::string* dest, StringPiece a, StringPiece b, StringPiece c) {
  ReserveAdditional(dest, a.size() + b.size() + c.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d,
               StringPiece e) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d,
               StringPiece e,
               StringPiece f) {
  ReserveAdditional(
      dest, a.size() + b.size() + c.size() + d.size() + e.size() + f.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d,
               StringPiece e,
               StringPiece f,
               StringPiece g) {
  ReserveAdditional(
      dest, a.size() + b.size() + c.size() + d.size() + e.size() + g.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d,
               StringPiece e,
               StringPiece f,
               StringPiece g,
               StringPiece h) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size() +
                              g.size() + h.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
  dest->append(h.data(), h.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d,
               StringPiece e,
               StringPiece f,
               StringPiece g,
               StringPiece h,
               StringPiece i) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size() +
                              g.size() + h.size() + i.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
  dest->append(h.data(), h.size());
  dest->append(i.data(), i.size());
}

void StrAppend(std::string* dest,
               StringPiece a,
               StringPiece b,
               StringPiece c,
               StringPiece d,
               StringPiece e,
               StringPiece f,
               StringPiece g,
               StringPiece h,
               StringPiece i,
               StringPiece j) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size() +
                              g.size() + h.size() + i.size() + j.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
  dest->append(h.data(), h.size());
  dest->append(i.data(), i.size());
  dest->append(j.data(), j.size());
}

// StrAppend 16-bit ------------------------------------------------------------

void StrAppend(string16* dest, StringPiece16 a, StringPiece16 b) {
  ReserveAdditional(dest, a.size() + b.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c) {
  ReserveAdditional(dest, a.size() + b.size() + c.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d,
               StringPiece16 e) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d,
               StringPiece16 e,
               StringPiece16 f) {
  ReserveAdditional(
      dest, a.size() + b.size() + c.size() + d.size() + e.size() + f.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d,
               StringPiece16 e,
               StringPiece16 f,
               StringPiece16 g) {
  ReserveAdditional(
      dest, a.size() + b.size() + c.size() + d.size() + e.size() + g.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d,
               StringPiece16 e,
               StringPiece16 f,
               StringPiece16 g,
               StringPiece16 h) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size() +
                              g.size() + h.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
  dest->append(h.data(), h.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d,
               StringPiece16 e,
               StringPiece16 f,
               StringPiece16 g,
               StringPiece16 h,
               StringPiece16 i) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size() +
                              g.size() + h.size() + i.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
  dest->append(h.data(), h.size());
  dest->append(i.data(), i.size());
}

void StrAppend(string16* dest,
               StringPiece16 a,
               StringPiece16 b,
               StringPiece16 c,
               StringPiece16 d,
               StringPiece16 e,
               StringPiece16 f,
               StringPiece16 g,
               StringPiece16 h,
               StringPiece16 i,
               StringPiece16 j) {
  ReserveAdditional(dest, a.size() + b.size() + c.size() + d.size() + e.size() +
                              g.size() + h.size() + i.size() + j.size());
  dest->append(a.data(), a.size());
  dest->append(b.data(), b.size());
  dest->append(c.data(), c.size());
  dest->append(d.data(), d.size());
  dest->append(e.data(), e.size());
  dest->append(f.data(), f.size());
  dest->append(g.data(), g.size());
  dest->append(h.data(), h.size());
  dest->append(i.data(), i.size());
  dest->append(j.data(), j.size());
}

// StrAppendInitializerList ----------------------------------------------------

void StrAppendInitializerList(std::string* dest,
                              std::initializer_list<StringPiece> pieces) {
  size_t additional_size = 0;
  for (const auto& cur : pieces)
    additional_size += cur.size();
  ReserveAdditional(dest, additional_size);

  for (const auto& cur : pieces)
    dest->append(cur.data(), cur.size());
}

void StrAppendInitializerList(string16* dest,
                              std::initializer_list<StringPiece16> pieces) {
  size_t additional_size = 0;
  for (const auto& cur : pieces)
    additional_size += cur.size();
  ReserveAdditional(dest, additional_size);

  for (const auto& cur : pieces)
    dest->append(cur.data(), cur.size());
}

}  // namespace base
