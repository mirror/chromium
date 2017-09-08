// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerMediaPlayerFactory_h
#define WorkerMediaPlayerFactory_h

#include <memory>

#include "core/CoreExport.h"

namespace blink {

class WebMediaPlayerFactory;
class WorkerClients;

class CORE_EXPORT WorkerMediaPlayerFactory {
 public:
  static void ProvideTo(WorkerClients*, std::unique_ptr<WebMediaPlayerFactory>);
  static WebMediaPlayerFactory* GetFrom(WorkerClients&);
};

}  // namespace blink

#endif  // WorkerMediaPlayerFactory_h
