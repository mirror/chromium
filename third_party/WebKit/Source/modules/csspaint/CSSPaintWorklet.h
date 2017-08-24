// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPaintWorklet_h
#define CSSPaintWorklet_h

#include "modules/ModulesExport.h"
#include "platform/heap/Handle.h"

namespace blink {

class Document;
class PaintWorklet;
class ScriptState;
class Worklet;

class MODULES_EXPORT CSSPaintWorklet {
 public:
  static Worklet* paintWorklet(ScriptState*);
  static PaintWorklet* paintWorklet(const Document&);
};

}  // namespace blink

#endif  // CSSPaintWorklet_h
