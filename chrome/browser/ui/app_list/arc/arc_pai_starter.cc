// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_pai_starter.h"

#include <memory>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/events/event_constants.h"

namespace arc {

ArcPaiStarter::ArcPaiStarter(Profile* profile) : profile_(profile) {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(profile_);
  // Prefs may not available in some unit tests.
  if (!prefs)
    return;
  prefs->AddObserver(this);
  MaybeStartPai();
}

ArcPaiStarter::~ArcPaiStarter() {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(profile_);
  if (!prefs)
    return;
  prefs->RemoveObserver(this);
}

// static
std::unique_ptr<ArcPaiStarter> ArcPaiStarter::CreateIfNeeded(Profile* profile) {
  if (profile->GetPrefs()->GetBoolean(prefs::kArcPaiStarted))
    return std::unique_ptr<ArcPaiStarter>();
  return base::MakeUnique<ArcPaiStarter>(profile);
}

void ArcPaiStarter::AcquireLock() {
  DCHECK(!locked_);
  locked_ = true;
}

void ArcPaiStarter::ReleaseLock() {
  DCHECK(locked_);
  locked_ = false;
  MaybeStartPai();
}

void ArcPaiStarter::MaybeStartPai() {
  if (started_ || locked_)
    return;

  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(profile_);
  DCHECK(prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      prefs->GetApp(kPlayStoreAppId);
  if (!app_info || !app_info->ready)
    return;

  started_ = true;
  StartPaiFlow();
  // TODO(khmel): Currently PAI flow is black-box for us. We can only start it
  // and rely that the Play Store will handle all cases. Ideally we need some
  // callback, notifying us that PAI flow finished successfully.
  profile_->GetPrefs()->SetBoolean(prefs::kArcPaiStarted, true);

  prefs->RemoveObserver(this);
}

void ArcPaiStarter::OnAppReadyChanged(const std::string& app_id, bool ready) {
  if (app_id == kPlayStoreAppId && ready)
    MaybeStartPai();
}

}  // namespace arc
