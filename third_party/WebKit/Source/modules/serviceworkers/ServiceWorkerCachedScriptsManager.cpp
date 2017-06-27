// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/ServiceWorkerCachedScriptsManager.h"

namespace blink {

void ServiceWorkerCachedScriptsManager::GetScriptTextAndMetaData(
    const KURL& script_url,
    std::unique_ptr<WTF::Function<void(String, std::unique_ptr<Vector<char>>)>>
        callback) {
  (*callback)(String(), nullptr);
}

}  // namespace blink
