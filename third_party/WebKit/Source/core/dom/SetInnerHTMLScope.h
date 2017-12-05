// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/wtf/Allocator.h"

#ifndef SetInnerHTMLScope_h
#define SetInnerHTMLScope_h

namespace blink {

class ContainerNode;

class SetInnerHTMLScope {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(SetInnerHTMLScope);

 public:
  SetInnerHTMLScope(ContainerNode*);

  ~SetInnerHTMLScope() { count_--; }

  static bool IsExecutingSetInnerHTML() { return count_; }

 private:
  static int count_;
};

}  // namespace blink

#endif  // SetInnerHTMLScope_h
