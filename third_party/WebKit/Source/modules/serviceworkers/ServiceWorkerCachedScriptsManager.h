// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ServiceWorkerCachedScriptsManager_h
#define ServiceWorkerCachedScriptsManager_h

#include "core/workers/WorkerCachedScriptsManager.h"

namespace blink {

class ServiceWorkerCachedScriptsManager : public WorkerCachedScriptsManager {
 public:
  ServiceWorkerCachedScriptsManager() : WorkerCachedScriptsManager() {}

  // Used by blink::WorkerThread.
  void GetScriptTextAndMetaData(
      const KURL& script_url,
      std::unique_ptr<
          WTF::Function<void(String, std::unique_ptr<Vector<char>>)>>) override;
};

}  // namespace blink

#endif  // ServiceWorkerCachedScriptsManager_h
