// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/arc/timer/arc_timer_impl.h"
#include "components/arc/timer/arc_timer_request.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

// ARC Timer Client sets timers by making Dbus calls to powerd.
class ArcTimerBridge : public KeyedService,
                       public InstanceHolder<mojom::TimerInstance>::Observer,
                       public mojom::TimerHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcTimerBridge* GetForBrowserContext(content::BrowserContext* context);

  ArcTimerBridge(content::BrowserContext* context,
                 ArcBridgeService* bridge_service);
  ~ArcTimerBridge() override;

  // InstanceHolder<mojom::TimerInstance>::Observer overrides.
  void OnInstanceReady() override;
  void OnInstanceClosed() override;

  // mojom::TimerHost overrides.
  void CreateTimers(std::vector<ArcTimerRequest> arc_timer_requests,
                    CreateTimersCallback callback) override;

 private:
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<mojom::TimerHost> binding_;

  // Object that manages all operations related to timers in the ARC instance.
  ArcTimerImpl arc_timer_impl_;

  base::WeakPtrFactory<ArcTimerBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_
