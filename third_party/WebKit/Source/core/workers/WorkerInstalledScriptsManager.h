// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerInstalledScriptsManager_h
#define WorkerInstalledScriptsManager_h

#include "platform/weborigin/KURL.h"
#include "platform/wtf/Optional.h"
#include "platform/wtf/Vector.h"

namespace blink {

class WorkerInstalledScriptsManager {
 public:
  WorkerInstalledScriptsManager() {}

  struct ScriptData {
    String source_text;
    std::unique_ptr<Vector<char>> meta_data;
  };

  // Used on the worker thread.
  virtual Optional<ScriptData> GetScriptData(const KURL& script_url) = 0;
};

}  // namespace blink

#endif  // WorkerInstalledScriptsManager_h
