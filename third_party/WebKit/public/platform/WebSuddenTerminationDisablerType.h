// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebSuddenTerminationDisablerType_h
#define WebSuddenTerminationDisablerType_h

namespace blink {

// Called when elements preventing the sudden termination of the frame become
// present or stop being present. |type| is the type of element (BeforeUnload
// handler, Unload handler).
typedef uint8_t WebSuddenTerminationDisablerType;

enum class WebSuddenTerminationDisabler : WebSuddenTerminationDisablerType {
  kBeforeUnloadHandler = 1 << 0,
  kUnloadHandler = 1 << 1,
  kLast,
};

}  // namespace blink

#endif  // WebSuddenTerminationDisablerType_h
