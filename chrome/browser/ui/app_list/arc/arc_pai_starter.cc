// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_pai_starter.h"

#include <memory>

#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "ui/events/event_constants.h"

namespace arc {

ArcPaiStarter::ArcPaiStarter(content::BrowserContext* context)
    : context_(context) {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  // Prefs may not available in some unit tests.
  if (!prefs)
    return;
  prefs->AddObserver(this);
  MaybeStartPai();
}

ArcPaiStarter::~ArcPaiStarter() {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  if (!prefs)
    return;
  prefs->RemoveObserver(this);
}

void ArcPaiStarter::AcquireLock() {
  ++lock_count_;
}

void ArcPaiStarter::ReleaseLock() {
  DCHECK_GT(lock_count_, 0);
  --lock_count_;
  MaybeStartPai();
}

void ArcPaiStarter::MaybeStartPai() {
  if (started_ || lock_count_)
    return;

  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  DCHECK(prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      prefs->GetApp(kPlayStoreAppId);
  if (!app_info || !app_info->ready)
    return;

  started_ = true;
  StartPaiFlow();
  prefs->RemoveObserver(this);
}

void ArcPaiStarter::OnAppReadyChanged(const std::string& app_id, bool ready) {
  if (app_id == kPlayStoreAppId && ready)
    MaybeStartPai();
}

}  // namespace arc
