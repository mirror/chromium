// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_launcher.h"

#include <memory>

#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "ui/events/event_constants.h"

ArcAppLauncher::ArcAppLauncher(content::BrowserContext* context,
                               const std::string& app_id,
                               const base::Optional<std::string>& launch_intent,
                               bool deferred_launch_allowed,
                               int64_t display_id)
    : context_(context),
      app_id_(app_id),
      launch_intent_(launch_intent),
      deferred_launch_allowed_(deferred_launch_allowed),
      display_id_(display_id) {
  if (MaybeLaunchApp())
    return;

  ArcAppListPrefs::Get(context_)->AddObserver(this);
  arc::ArcServiceManager::Get()
      ->arc_bridge_service()
      ->intent_helper()
      ->AddObserver(this);
}

ArcAppLauncher::~ArcAppLauncher() {
  if (app_launched_)
    return;

  VLOG(2) << "App " << app_id_ << "was not launched.";
  RemoveObservers();
}

void ArcAppLauncher::OnAppRegistered(
    const std::string& app_id,
    const ArcAppListPrefs::AppInfo& app_info) {
  if (app_id == app_id_)
    MaybeLaunchApp();
}

void ArcAppLauncher::OnAppReadyChanged(const std::string& app_id, bool ready) {
  if (app_id == app_id_)
    MaybeLaunchApp();
}

void ArcAppLauncher::OnConnectionReady() {
  MaybeLaunchApp();
}

bool ArcAppLauncher::MaybeLaunchApp() {
  DCHECK(!app_launched_);

  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  DCHECK(prefs);

  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info = prefs->GetApp(app_id_);
  if (!app_info)
    return false;

  if (launch_intent_.has_value() && !arc::ArcServiceManager::Get()
                                         ->arc_bridge_service()
                                         ->intent_helper()
                                         ->IsConnected()) {
    return false;
  }

  if (!app_info->ready && !deferred_launch_allowed_)
    return false;

  RemoveObservers();

  if (!arc::LaunchAppWithIntent(context_, app_id_, launch_intent_, ui::EF_NONE,
                                display_id_)) {
    VLOG(2) << "Failed to launch app: " + app_id_ + ".";
  }

  app_launched_ = true;
  return true;
}

void ArcAppLauncher::RemoveObservers() {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  if (prefs)
    prefs->RemoveObserver(this);
  arc::ArcServiceManager* arc_service_manager = arc::ArcServiceManager::Get();
  if (arc_service_manager)
    arc_service_manager->arc_bridge_service()->intent_helper()->RemoveObserver(
        this);
}
