// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/task_runner_util.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/timer/arc_timer.h"
#include "components/arc/timer/arc_timer_bridge.h"
#include "components/arc/timer/arc_timer_traits.h"

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

bool IsSupportedClock(int32_t clock_id) {
  return clock_id == CLOCK_BOOTTIME_ALARM || clock_id == CLOCK_REALTIME_ALARM;
}

// Creates timers with the given arguments. Returns base::nullopt on failure.
// On success, returns a non-empty vector of |TimersAndProxies| objects.
base::Optional<ArcTimerBridge::TimersAndProxies> CreateArcTimers(
    bool timers_already_created,
    std::vector<arc::ArcTimerRequest> arc_timer_requests) {
  DVLOG(1) << "Received request to create ARC timers";
  if (timers_already_created) {
    LOG(ERROR) << "Double creation not supported";
    return base::nullopt;
  }

  // Iterate over the list of {clock_id, expiration_fd} and create an |ArcTimer|
  // and |mojom::TimerPtrInfo| entry for each clock.
  std::set<int32_t> seen_clocks;
  ArcTimerBridge::TimersAndProxies result;
  for (auto& request : arc_timer_requests) {
    // Read each entry one by one. Each entry will have an |ArcTimer| entry
    // associated with it.
    int32_t clock_id = request.clock_id;

    if (!IsSupportedClock(clock_id)) {
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

    mojom::TimerPtrInfo timer_proxy_info;
    // TODO(b/69759087): Make |ArcTimer| take |clock_id| to create timers of
    // different clock types.
    // The instance opens clocks of type CLOCK_BOOTTIME_ALARM and
    // CLOCK_REALTIME_ALARM. However, it uses only CLOCK_BOOTTIME_ALARM to set
    // wake up alarms. At this point, it's okay to pretend the host supports
    // CLOCK_REALTIME_ALARM instead of returning an error.
    result.clocks.push_back(clock_id);
    result.timers.push_back(std::make_unique<ArcTimer>(
        std::move(expiration_fd), mojo::MakeRequest(&timer_proxy_info)));
    result.proxies.push_back(std::move(timer_proxy_info));
  }
  return result;
}

}  // namespace

// static
BrowserContextKeyedServiceFactory* ArcTimerBridge::GetFactory() {
  return ArcTimerBridgeFactory::GetInstance();
}

// static
ArcTimerBridge* ArcTimerBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcTimerBridgeFactory::GetForBrowserContext(context);
}

ArcTimerBridge::ArcTimerBridge(content::BrowserContext* context,
                               ArcBridgeService* bridge_service)
    : sequenced_task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      arc_bridge_service_(bridge_service),
      arc_supported_timer_clocks_({CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM}),
      binding_(this),
      weak_ptr_factory_(this) {
  arc_bridge_service_->timer()->SetHost(this);
  arc_bridge_service_->timer()->AddObserver(this);
}

ArcTimerBridge::~ArcTimerBridge() {
  arc_bridge_service_->timer()->RemoveObserver(this);
  arc_bridge_service_->timer()->SetHost(nullptr);
}

ArcTimerBridge::TimersAndProxies::TimersAndProxies() = default;
ArcTimerBridge::TimersAndProxies::~TimersAndProxies() = default;
ArcTimerBridge::TimersAndProxies::TimersAndProxies& operator=(
    ArcTimerBridge::TimersAndProxies&&) = default;

void ArcTimerBridge::OnConnectionReady() {}

void ArcTimerBridge::OnConnectionClosed() {
  DVLOG(1) << "OnConnectionClosed";
  DeleteArcTimers();
}

void ArcTimerBridge::CreateTimers(
    std::vector<ArcTimerRequest> arc_timer_requests,
    CreateTimersCallback callback) {
  DVLOG(1) << "Received CreateTimers";
  // Alarm timers can't be created on the UI thread. Post a task to create
  // timers.
  base::PostTaskAndReplyWithResult(
      sequenced_task_runner_.get(), FROM_HERE,
      base::BindOnce(&CreateArcTimers, !arc_timers_.empty(),
                     std::move(arc_timer_requests)),
      base::BindOnce(&ArcTimerBridge::OnArcTimersCreated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ArcTimerBridge::OnArcTimersCreated(
    CreateTimersCallback callback,
    base::Optional<TimersAndProxies> timers_and_proxies) {
  if (timers_and_proxies == base::nullopt) {
    std::move(callback).Run(base::nullopt);
    return;
  }
  DCHECK(timers_and_proxies->clocks.size() ==
         timers_and_proxies->timers.size() ==
         timers_and_proxies->proxies.size());
  arc_timers_ = std::move(timers_and_proxies->timers);
  std::vector<mojom::ArcTimerResponsePtr> result;
  for (size_t i = 0; i < timers_and_proxies->clocks.size(); i++) {
    mojom::ArcTimerResponsePtr response = mojom::ArcTimerResponse::New();
    response->clock_id =
        mojo::EnumTraits<arc::mojom::ClockId, int32_t>::ToMojom(
            timers_and_proxies->clocks[i]);
    auto& proxy = timers_and_proxies->proxies[i];
    response->timer = std::move(proxy);
    result.push_back(std::move(response));
  }
  std::move(callback).Run(std::move(result));
}

void ArcTimerBridge::DeleteArcTimers() {
  sequenced_task_runner_->PostTask(
      FROM_HERE, base::BindOnce([](std::vector<std::unique_ptr<ArcTimer>>) {},
                                std::move(arc_timers_)));
}

}  // namespace arc
