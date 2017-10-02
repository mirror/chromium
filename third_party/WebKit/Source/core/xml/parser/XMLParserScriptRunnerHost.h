// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XMLParserScriptRunnerHost_h
#define XMLParserScriptRunnerHost_h

#include "core/CoreExport.h"
#include "platform/wtf/Forward.h"

namespace blink {

class CORE_EXPORT XMLParserScriptRunnerHost : public GarbageCollectedMixin {
 public:
  virtual ~XMLParserScriptRunnerHost() {}
  DEFINE_INLINE_VIRTUAL_TRACE() {}

  virtual void NotifyScriptLoaded() = 0;
};

}  // namespace blink

#endif
