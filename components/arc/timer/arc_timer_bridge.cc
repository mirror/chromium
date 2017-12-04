// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/timer/arc_timer.h"
#include "components/arc/timer/arc_timer_bridge.h"

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
      arc_supported_timer_clocks_({CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM}),
      binding_(this),
      weak_ptr_factory_(this) {
  arc_bridge_service_->timer()->AddObserver(this);
}

ArcTimerBridge::~ArcTimerBridge() {
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
  DeleteArcTimers();
}

void ArcTimerBridge::CreateTimers(
    std::vector<ArcTimerRequest> arc_timer_requests,
    CreateTimersCallback callback) {
  DVLOG(1) << "Received CreateTimers";
  std::move(callback).Run(CreateArcTimers(std::move(arc_timer_requests)));
}

base::Optional<std::vector<mojom::TimerPtr>> ArcTimerBridge::CreateArcTimers(
    std::vector<arc::ArcTimerRequest> arc_timer_requests) {
  DVLOG(1) << "Received request to create ARC timers";

  if (!arc_timers_.empty()) {
    LOG(ERROR) << "Double creation not supported";
    return base::nullopt;
  }

  // Iterate over the list of {clock_id, expiration_fd} and create an |ArcTimer|
  // entry for each clock.
  std::set<std::unique_ptr<ArcTimer>> arc_timers;
  std::set<int32_t> seen_clocks;
  std::vector<mojom::TimerPtr> result;
  for (auto& request : arc_timer_requests) {
    // Read each entry one by one. Each entry will have an |ArcTimer| entry
    // associated with it.
    int32_t clock_id = request.clock_id;

    if (arc_supported_timer_clocks_.find(clock_id) ==
        arc_supported_timer_clocks_.end()) {
      LOG(ERROR) << "Unsupported clock=" << clock_id;
      return base::nullopt;
    }

    if (!seen_clocks.insert(clock_id).second) {
      LOG(ERROR) << "Duplicate clocks not supported";
      return base::nullopt;
    }

    base::ScopedFD expiration_fd = std::move(request.expiration_fd);
    if (!expiration_fd.is_valid()) {
      LOG(ERROR) << "Bad file descriptor for clock=" << clock_id;
      return base::nullopt;
    }

    mojom::TimerPtr timer_proxy;
    mojo::InterfaceRequest<mojom::Timer> interface_request =
        mojo::MakeRequest(&timer_proxy);
    // TODO(b/69759087): Make |ArcTimer| take |clock_id| to create timers of
    // different clock types.
    // The instance opens clocks of type CLOCK_BOOTTIME_ALARM and
    // CLOCK_REALTIME_ALARM. However, it uses only CLOCK_BOOTTIME_ALARM to set
    // wake up alarms. At this point, it's okay to pretend the host supports
    // CLOCK_REALTIME_ALARM instead of returning an error.
    arc_timers.insert(std::make_unique<ArcTimer>(std::move(expiration_fd),
                                                 std::move(interface_request)));
    result.push_back(std::move(timer_proxy));
  }
  arc_timers_ = std::move(arc_timers);
  return result;
}

void ArcTimerBridge::DeleteArcTimers() {
  arc_timers_.clear();
}

}  // namespace arc
