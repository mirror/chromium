// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_service_manager.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task_runner.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_session.h"
#include "components/arc/arc_session_runner.h"
#include "components/arc/intent_helper/arc_intent_helper_observer.h"

namespace arc {
namespace {

// Weak pointer.  This class is owned by arc::ArcServiceLauncher.
ArcServiceManager* g_arc_service_manager = nullptr;

// This pointer is owned by ArcServiceManager.
ArcSessionRunner* g_arc_session_runner_for_testing = nullptr;

}  // namespace

class ArcServiceManager::IntentHelperObserverImpl
    : public ArcIntentHelperObserver {
 public:
  explicit IntentHelperObserverImpl(ArcServiceManager* manager);
  ~IntentHelperObserverImpl() override = default;

 private:
  void OnIntentFiltersUpdated() override;
  ArcServiceManager* const manager_;

  DISALLOW_COPY_AND_ASSIGN(IntentHelperObserverImpl);
};

ArcServiceManager::IntentHelperObserverImpl::IntentHelperObserverImpl(
    ArcServiceManager* manager)
    : manager_(manager) {}

void ArcServiceManager::IntentHelperObserverImpl::OnIntentFiltersUpdated() {
  DCHECK(manager_->thread_checker_.CalledOnValidThread());
  for (auto& observer : manager_->observer_list_)
    observer.OnIntentFiltersUpdated();
}

ArcServiceManager::ArcServiceManager(
    scoped_refptr<base::TaskRunner> blocking_task_runner)
    : blocking_task_runner_(blocking_task_runner),
      intent_helper_observer_(base::MakeUnique<IntentHelperObserverImpl>(this)),
      icon_loader_(new ActivityIconLoader()),
      activity_resolver_(new LocalActivityResolver()) {
  DCHECK(!g_arc_service_manager);
  g_arc_service_manager = this;

  arc_bridge_service_ = base::MakeUnique<ArcBridgeService>();
  if (g_arc_session_runner_for_testing) {
    arc_bridge_service_->InitializeArcSessionRunner(
        base::WrapUnique(g_arc_session_runner_for_testing));
    g_arc_session_runner_for_testing = nullptr;
  } else {
    arc_bridge_service_->InitializeArcSessionRunner(
        base::MakeUnique<ArcSessionRunner>(base::Bind(&ArcSession::Create,
                                                      arc_bridge_service_.get(),
                                                      blocking_task_runner)));
  }
}

ArcServiceManager::~ArcServiceManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(g_arc_service_manager == this);
  g_arc_service_manager = nullptr;
  if (g_arc_session_runner_for_testing)
    delete g_arc_session_runner_for_testing;
}

// static
ArcServiceManager* ArcServiceManager::Get() {
  if (!g_arc_service_manager)
    return nullptr;
  DCHECK(g_arc_service_manager->thread_checker_.CalledOnValidThread());
  return g_arc_service_manager;
}

ArcBridgeService* ArcServiceManager::arc_bridge_service() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return arc_bridge_service_.get();
}

void ArcServiceManager::AddService(std::unique_ptr<ArcService> service) {
  DCHECK(thread_checker_.CalledOnValidThread());
  services_.emplace_back(std::move(service));
}

void ArcServiceManager::AddObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observer_list_.AddObserver(observer);
}

void ArcServiceManager::RemoveObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observer_list_.RemoveObserver(observer);
}

void ArcServiceManager::Shutdown() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Before actual shutdown, notify observers for clean up.
  for (auto& observer : observer_list_)
    observer.OnArcShutdown();

  icon_loader_ = nullptr;
  activity_resolver_ = nullptr;
  services_.clear();
  arc_bridge_service_->OnShutdown();
}

// static
void ArcServiceManager::SetArcSessionRunnerForTesting(
    std::unique_ptr<ArcSessionRunner> arc_session_runner) {
  if (g_arc_session_runner_for_testing)
    delete g_arc_session_runner_for_testing;
  g_arc_session_runner_for_testing = arc_session_runner.release();
}

}  // namespace arc
