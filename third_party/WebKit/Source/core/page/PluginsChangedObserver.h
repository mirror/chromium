// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PluginsChangedObserver_h
#define PluginsChangedObserver_h

#include "core/CoreExport.h"

namespace blink {

class Page;

class CORE_EXPORT PluginsChangedObserver {
 public:
  virtual void PluginsChanged() = 0;

 protected:
  explicit PluginsChangedObserver(Page*);
  ~PluginsChangedObserver();

 private:
  Page* page_;
};

}  // namespace blink

#endif  // PluginsChangedObserver_h
