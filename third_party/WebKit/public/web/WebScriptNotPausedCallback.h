// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebScriptNotPausedCallback_h
#define WebScriptNotPausedCallback_h

namespace blink {

// A helper class to be notified when script is not paused.
// TODO(devlin): Replace this with a base::OnceCallback where it's used when
// base::OnceCallback is allowed in blink.
class WebScriptNotPausedCallback {
 public:
  enum Result {
    // The context was invalid or destroyed.
    CONTEXT_INVALID_OR_DESTROYED,
    // Script is not paused.
    READY,
  };

  virtual ~WebScriptNotPausedCallback() {}

  // Invoked when script is not paused.
  virtual void Run(Result) = 0;
};

}  // namespace blink

#endif  // WebScriptNotPausedCallback_h
