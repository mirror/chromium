// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontObjectsCount_h
#define FontObjectsCount_h

#include "platform/PlatformExport.h"
#include "base/logging.h"

// See http://www.drdobbs.com/cpp/counting-objects-in-c/184403484

namespace blink {

class FontPlatformData;
class SimpleFontData;
class FontDescription;

template <typename T>
inline const char* typeName(void) {
  return "unknown";
}

template <>
inline const char* typeName<FontPlatformData>(void) {
  return "FontPlatformData";
}

template <>
inline const char* typeName<SimpleFontData>(void) {
  return "SimpleFontData";
}

template <>
inline const char* typeName<FontDescription>(void) {
  return "FontDescription";
}

template <class Obj>
class PLATFORM_EXPORT CountedFontObj
{
public:
  CountedFontObj() {
    ++allocations_;
    LogCount();
  }

  CountedFontObj(const CountedFontObj& obj) {
    if (this == &obj)
      return;
    ++allocations_;
    ++copies_;
    LogCount();
  }

  virtual ~CountedFontObj() {
    ++frees_;
    LogCount();
  }

 private:
   static size_t allocations_;
   static size_t copies_;
   static size_t frees_;

  void LogCount() {
    VLOG(4) << "Num " << typeName<Obj>()
            << " objects: " << allocations_ - frees_
            << " alloc's (copies): " << allocations_ << "(" << copies_ << ") freed: " << frees_;
  }
};

template <typename T> size_t CountedFontObj<T>::allocations_ = 0;
template <typename T> size_t CountedFontObj<T>::frees_ = 0;
template <typename T> size_t CountedFontObj<T>::copies_ = 0;

};

#endif
