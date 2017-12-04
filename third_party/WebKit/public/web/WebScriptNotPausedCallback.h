// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebScriptNotPausedCallback_h
#define WebScriptNotPausedCallback_h

namespace blink {

// A helper class to be notified when script is not paused.
// TODO(devlin): Replace this with a base::Callback where it's used when
// base::Callback is allowed in blink.
class WebScriptNotPausedCallback {
 public:
  enum Result {
    CONTEXT_INVALID,    // The context was invalid.
    CONTEXT_DESTROYED,  // The context was destroyed before script was unpaused.
    READY,              // Script is not paused.
  };

  virtual ~WebScriptNotPausedCallback() {}

  // Invoked when script is not paused.
  virtual void Completed(Result) = 0;
};

}  // namespace blink

#endif  // WebScriptNotPausedCallback_h
