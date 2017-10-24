// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/arc/timer/arc_timer_bridge.h"
#include "components/arc/timer/arc_timer_impl.h"

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_service_manager.h"

namespace arc {

namespace {

// Singleton factory for ArcTimerBridge.
class ArcTimerBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcTimerBridge,
          ArcTimerBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcTimerBridgeFactory";

  static ArcTimerBridgeFactory* GetInstance() {
    return base::Singleton<ArcTimerBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcTimerBridgeFactory>;
  ArcTimerBridgeFactory() = default;
  ~ArcTimerBridgeFactory() override = default;
};

}  // namespace

// static
ArcTimerBridge* ArcTimerBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcTimerBridgeFactory::GetForBrowserContext(context);
}

ArcTimerBridge::ArcTimerBridge(content::BrowserContext* context,
                               ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      weak_ptr_factory_(this) {
  arc_bridge_service_->timer()->AddObserver(this);
}

ArcTimerBridge::~ArcTimerBridge() {
  arc_timer_impl_.DeleteArcTimers();
  arc_bridge_service_->timer()->RemoveObserver(this);
}

void ArcTimerBridge::OnInstanceReady() {
  mojom::TimerInstance* timer_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->timer(), Init);
  DCHECK(timer_instance);
  mojom::TimerHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  timer_instance->Init(std::move(host_proxy));
}

void ArcTimerBridge::OnInstanceClosed() {
  DVLOG(1) << "OnInstanceClosed";
  arc_timer_impl_.DeleteArcTimers();
}

void ArcTimerBridge::CreateTimers(std::vector<ArcTimerArgs> args,
                                  CreateTimersCallback callback) {
  DVLOG(1) << "Received CreateTimers";
  std::move(callback).Run(arc_timer_impl_.CreateArcTimers(std::move(args)));
}

void ArcTimerBridge::SetTimer(int32_t clock_id,
                              int64_t seconds,
                              int64_t nanoseconds,
                              SetTimerCallback callback) {
  DVLOG(1) << "Received SetTimer"
           << " ClockId: " << clock_id << " Seconds: " << seconds
           << " Nanoseconds: " << nanoseconds;
  std::move(callback).Run(arc_timer_impl_.SetArcTimer(
      clock_id, base::TimeDelta::FromSecondsD(
                    seconds + (static_cast<double>(nanoseconds) /
                               base::Time::kNanosecondsPerSecond))));
}

}  // namespace arc
