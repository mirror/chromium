// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorBaseAgent.h"

#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/Vector.h"

namespace blink {

using SessionInitCallbackVector =
    WTF::Vector<InspectorAgent::SessionInitCallback>;
SessionInitCallbackVector& GetSessionInitCallbackVector() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(SessionInitCallbackVector,
                                  session_init_vector, ());
  return session_init_vector;
}

void InspectorAgent::RegisterSessionInitCallback(SessionInitCallback cb) {
  GetSessionInitCallbackVector().push_back(cb);
}

void InspectorAgent::CallSessionInitCallbacks(InspectorSession* session,
                                              const SessionInitArgs* args) {
  for (auto& cb : GetSessionInitCallbackVector()) {
    cb(session, args);
  }
}

}  // namespace blink
