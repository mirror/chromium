// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ControllerFactory_h
#define ControllerFactory_h

#include "core/CoreExport.h"
#include "public/platform/WebPageVisibilityState.h"
#include "public/web/WebSandboxFlags.h"

namespace blink {

class WebDevToolsAgentClient;
class WebDevToolsAgentCore;
class WebLocalFrameBase;

// ControllerFactory allows access to controller/ instantiated objects.

class CORE_EXPORT ControllerFactory {
 public:
  static ControllerFactory& GetInstance();

  virtual WebDevToolsAgentCore* CreateWebDevToolsAgent(
      WebLocalFrameBase*,
      WebDevToolsAgentClient*) const = 0;

 protected:
  // Takes ownership of |factory|.
  static void SetInstance(ControllerFactory&);

 private:
  static ControllerFactory* factory_instance_;
};

}  // namespace blink

#endif
