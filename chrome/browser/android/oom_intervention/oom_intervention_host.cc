// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/oom_intervention_host.h"

#include "chrome/browser/android/oom_intervention/oom_intervention_infobar_delegate.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(OomInterventionHost);

namespace {

void OnConnectionError(uint32_t custom_reason, const std::string& description) {
  LOG(ERROR) << "reason: " << custom_reason << ", description: " << description;
}

}  // namespace

OomInterventionHost::OomInterventionHost(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), suspended_(false) {}

OomInterventionHost::~OomInterventionHost() {}

void OomInterventionHost::AcceptIntervention() {
  // TODO(bashi): Record stats
}

void OomInterventionHost::DeclineIntervention() {
  // TODO(bashi): Record stats

  if (oom_intervention_.is_bound()) {
    oom_intervention_->OnInterventionDeclined();
  }
}

void OomInterventionHost::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  LOG(ERROR) << "RenderFrameCreated";
  render_frame_host->GetRemoteInterfaces()->GetInterface(&oom_intervention_);
  oom_intervention_.set_connection_error_with_reason_handler(
      base::BindOnce(&OnConnectionError));
  CHECK(oom_intervention_.is_bound());
  NearOomMonitor::GetInstance()->Register(this);
}

void OomInterventionHost::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  LOG(ERROR) << "RenderFrameDeleted";
  NearOomMonitor::GetInstance()->Unregister(this);
}

void OomInterventionHost::WebContentsDestroyed() {
  LOG(ERROR) << "WebContentsDestroyed";
}

void OomInterventionHost::OnNearOomDetected() {
  LOG(ERROR) << "OnNearOomDetected";
  if (!oom_intervention_.is_bound()) {
    LOG(ERROR) << "Not bound to the interface";
    return;
  }
  if (suspended_) {
    LOG(ERROR) << "Already suspended";
    return;
  }
  suspended_ = true;
  oom_intervention_->OnNearOomDetected();
  NearOomMonitor::GetInstance()->Unregister(this);

  // TODO(bashi): Wait for a response from the renderer before showing InfoBar?
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents());
  OomInterventionInfoBarDelegate::Create(infobar_service, this);
}
