// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/arc/timer/arc_timer_bridge.h"

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

mojom::ArcTimerResult ConvertDbusResultToMojoResult(
    power_manager::ArcTimerResult result) {
  switch (result) {
    case power_manager::ArcTimerResult::ARC_TIMER_RESULT_SUCCESS:
      return mojom::ArcTimerResult::SUCCESS;

    default:
      return mojom::ArcTimerResult::FAILURE;
  }
}

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
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
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

void ArcTimerBridge::DeleteArcTimersCallback(
    power_manager::ArcTimerResult result) {
  DVLOG(2) << "Received DeleteArcTimersCallback result="
           << static_cast<int32_t>(result);
}

void ArcTimerBridge::OnInstanceClosed() {
  DVLOG(2) << "OnInstanceClosed";
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->DeleteArcTimers(
      base::Bind(&ArcTimerBridge::DeleteArcTimersCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ArcTimerBridge::CreateArcTimersCallback(
    CreateTimersCallback callback,
    power_manager::ArcTimerResult result) {
  DVLOG(2) << "Received CreateArcTimersCallback result="
           << static_cast<int32_t>(result);
  std::move(callback).Run(ConvertDbusResultToMojoResult(result));
}

void ArcTimerBridge::CreateTimers(
    const std::vector<chromeos::PowerManagerClient::ArcTimerArgs>&
        arc_timers_args,
    CreateTimersCallback callback) {
  DVLOG(2) << "Received CreateTimers";
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->CreateArcTimers(
      arc_timers_args,
      base::BindOnce(&ArcTimerBridge::CreateArcTimersCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ArcTimerBridge::SetArcTimerCallback(SetTimerCallback callback,
                                         power_manager::ArcTimerResult result) {
  DVLOG(2) << "Received SetArcTimerCallback result="
           << static_cast<int32_t>(result);
  std::move(callback).Run(ConvertDbusResultToMojoResult(result));
}

void ArcTimerBridge::SetTimer(int32_t clock_id,
                              int64_t seconds,
                              int64_t nanoseconds,
                              SetTimerCallback callback) {
  DVLOG(2) << "Received SetTimer";
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->SetArcTimer(
      clock_id, seconds, nanoseconds,
      base::BindOnce(&ArcTimerBridge::SetArcTimerCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

// Overriden from chromeos::PowerManagerClient::Observer.
void ArcTimerBridge::PowerManagerRestarted() {
  DVLOG(2) << "Powerd restarted";
  auto* instance = ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->timer(),
                                               OnPowerdRespawn);
  if (!instance) {
    DVLOG(1) << "No instance to notify";
    return;
  }
  instance->OnPowerdRespawn();
}

}  // namespace arc
