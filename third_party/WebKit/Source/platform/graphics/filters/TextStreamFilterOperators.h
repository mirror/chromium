// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextStreamFilterOperators_h
#define TextStreamFilterOperators_h

namespace blink {

class TextStream;
class FloatPoint3D;

TextStream& operator<<(TextStream&, const FloatPoint3D&);
}  // namespace blink

#endif  // TextStreamFilterOperators_h
