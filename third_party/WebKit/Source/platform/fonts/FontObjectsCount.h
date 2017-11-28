// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontObjectsCount_h
#define FontObjectsCount_h

#include "platform/PlatformExport.h"
#include "base/logging.h"

namespace blink {

class FontPlatformData;
class SimpleFontData;

template <typename T>
inline const char* typeName(void) { return "unknown"; }

template <>
inline const char* typeName<FontPlatformData>(void) { return "FontPlatformData"; }

template <>
inline const char* typeName<SimpleFontData>(void) { return "SimpleFontData"; }

template <class Obj>
class PLATFORM_EXPORT CountedFontObj
{
public:
  CountedFontObj() {
    ++CounterReference();
    LogCount();
  }
  CountedFontObj(const CountedFontObj& obj) {
    if (this != &obj)
      ++CounterReference();
    LogCount();}
  ~CountedFontObj() {
    --CounterReference();
    LogCount();
  }

  static size_t OustandingObjects() { return CounterReference(); }

 private:
  static long& CounterReference() {
    static long counter(0);
    return counter;
   }

   void LogCount() {
     LOG(INFO) << "Num " << typeName<Obj>()
               << " objects: " << CounterReference();
   }

};

};

#endif
