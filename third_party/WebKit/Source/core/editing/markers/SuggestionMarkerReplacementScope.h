// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SuggestionMarkerReplacementScope_h
#define SuggestionMarkerReplacementScope_h

#include "platform/heap/Handle.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class DocumentMarkerController;

class SuggestionMarkerReplacementScope {
  WTF_MAKE_NONCOPYABLE(SuggestionMarkerReplacementScope);
  STACK_ALLOCATED();

 public:
  explicit SuggestionMarkerReplacementScope(DocumentMarkerController*);
  ~SuggestionMarkerReplacementScope();

 private:
  Persistent<DocumentMarkerController> document_marker_controller_;
};

}  // namespace blink

#endif
