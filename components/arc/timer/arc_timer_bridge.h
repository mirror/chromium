// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/arc/timer/arc_timer.h"
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
  // Creates timers with the given arguments. Returns base::nullopt on failure.
  // On success, returns a non-empty vector of |Timer| objects.
  base::Optional<std::vector<mojom::TimerPtr>> CreateArcTimers(
      std::vector<arc::ArcTimerRequest> arc_timer_requests);

  // Deletes all timers created i.e. any pending timers are cancelled.
  void DeleteArcTimers();

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  // The type of clocks whose timers can be created for the instance. Currently,
  // only CLOCK_REALTIME_ALARM and CLOCK_BOOTTIME_ALARM is supported.
  const std::set<int32_t> arc_supported_timer_clocks_;

  // Store of |ArcTimer| objects.
  std::set<std::unique_ptr<ArcTimer>> arc_timers_;

  mojo::Binding<mojom::TimerHost> binding_;
  base::WeakPtrFactory<ArcTimerBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_
