// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentElementCounts_h
#define DocumentElementCounts_h

namespace blink {

#define ELEMENT_COUNTERS(V) \
  V(HTMLFormElement)        \
  V(HTMLDataListElement)    \
  V(HTMLFieldSetElement)

#define ELEMENT_TYPE_COUNTER_METHODS(T)        \
 private:                                      \
  int T##_count_ = 0;                          \
                                               \
 public:                                       \
  void Increment##T##Count() { T##_count_++; } \
  void Decrement##T##Count() { T##_count_--; } \
  int T##Count() { return T##_count_; }        \
                                               \
 private:

#define COUNTER_CHANGE_OVERLOADS(T)                            \
  void IncrementTypeCountInDocument() const final {            \
    GetDocument().Increment##T##Count();                       \
  }                                                            \
  void DecrementTypeCountInDocument() const final {            \
    GetDocument().Decrement##T##Count();                       \
  }                                                            \
  static int GetElementCountFromDocument(Document& document) { \
    return document.T##Count();                                \
  }                                                            \
  static constexpr bool kIsCountedByDocument = true;

}  // namespace blink

#endif  // DocumentElementCounts_h
