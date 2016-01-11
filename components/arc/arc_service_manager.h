// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_SERVICE_MANAGER_H_
#define COMPONENTS_ARC_ARC_SERVICE_MANAGER_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/thread_checker.h"

namespace arc {

class ArcAuthService;
class ArcBridgeService;
class ArcClipboardBridge;
class ArcInputBridge;
class ArcSettingsBridge;
class ArcPowerBridge;

// Manages creation and destruction of services that communicate with the ARC
// instance via the ArcBridgeService.
class ArcServiceManager {
 public:
  ArcServiceManager(scoped_ptr<ArcAuthService> auth_service,
                    scoped_ptr<ArcSettingsBridge> settings_bridge);
  virtual ~ArcServiceManager();

  // |arc_bridge_service| can only be accessed on the thread that this
  // class was created on.
  ArcBridgeService* arc_bridge_service();

  // Gets the global instance of the ARC Service Manager. This can only be
  // called on the thread that this class was created on.
  static ArcServiceManager* Get();

 private:
  base::ThreadChecker thread_checker_;
  scoped_ptr<ArcBridgeService> arc_bridge_service_;

  // Individual services
  scoped_ptr<ArcAuthService> arc_auth_service_;
  scoped_ptr<ArcClipboardBridge> arc_clipboard_bridge_;
  scoped_ptr<ArcInputBridge> arc_input_bridge_;
  scoped_ptr<ArcSettingsBridge> arc_settings_bridge_;
  scoped_ptr<ArcPowerBridge> arc_power_bridge_;

  DISALLOW_COPY_AND_ASSIGN(ArcServiceManager);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_SERVICE_MANAGER_H_
