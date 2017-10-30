// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/oom_intervention_tab_helper.h"

#include "base/metrics/histogram_macros.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(OomInterventionTabHelper);

// This enum is associated with UMA. Values must be kept in sync with enums.xml
// and must not be renumbered/reused.
enum class NearOomMonitoringEndReason {
  OOM_PROTECTED_CRASH = 0,
  RENDERER_GONE = 1,
  NAVIGATION = 2,
  COUNT,
};

// static
bool OomInterventionTabHelper::IsEnabled() {
  NearOomMonitor* monitor = NearOomMonitor::GetInstance();
  return monitor && monitor->IsRunning();
}

OomInterventionTabHelper::OomInterventionTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

OomInterventionTabHelper::~OomInterventionTabHelper() = default;

void OomInterventionTabHelper::WebContentsDestroyed() {
  StopMonitoring();
}

void OomInterventionTabHelper::RenderProcessGone(
    base::TerminationStatus status) {
  if (near_oom_detected_time_) {
    base::TimeDelta elapsed_time =
        base::TimeTicks::Now() - near_oom_detected_time_.value();
    if (status == base::TERMINATION_STATUS_OOM_PROTECTED) {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "Memory.Experimental.OomIntervention."
          "OomProtectedCrashAfterDetectionTime",
          elapsed_time);
    } else {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "Memory.Experimental.OomIntervention."
          "RendererGoneAfterDetectionTime",
          elapsed_time);
    }
    near_oom_detected_time_.reset();
  } else if (subscription_) {
    if (status == base::TERMINATION_STATUS_OOM_PROTECTED) {
      UMA_HISTOGRAM_ENUMERATION(
          "Memory.Experimental.OomIntervention.NearOomMonitoringEndReason",
          NearOomMonitoringEndReason::OOM_PROTECTED_CRASH,
          NearOomMonitoringEndReason::COUNT);
    } else {
      UMA_HISTOGRAM_ENUMERATION(
          "Memory.Experimental.OomIntervention.NearOomMonitoringEndReason",
          NearOomMonitoringEndReason::RENDERER_GONE,
          NearOomMonitoringEndReason::COUNT);
    }
  }
}

void OomInterventionTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;

  if (navigation_handle->IsSameDocument())
    return;

  if (!navigation_started_)
    return;

  navigation_started_ = true;
  if (near_oom_detected_time_) {
    base::TimeDelta elapsed_time =
        base::TimeTicks::Now() - near_oom_detected_time_.value();
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "Memory.Experimental.OomIntervention."
        "NavigatedAfterDetectionTime",
        elapsed_time);
    near_oom_detected_time_.reset();
  } else if (subscription_) {
    UMA_HISTOGRAM_ENUMERATION(
        "Memory.Experimental.OomIntervention.NearOomMonitoringEndReason",
        NearOomMonitoringEndReason::NAVIGATION,
        NearOomMonitoringEndReason::COUNT);
  }

  StartMonitoringIfNeeded();
}

void OomInterventionTabHelper::WasShown() {
  StartMonitoringIfNeeded();
}

void OomInterventionTabHelper::WasHidden() {
  StopMonitoring();
}

void OomInterventionTabHelper::StartMonitoringIfNeeded() {
  if (subscription_)
    return;

  if (near_oom_detected_time_)
    return;

  subscription_ = NearOomMonitor::GetInstance()->RegisterCallback(base::Bind(
      &OomInterventionTabHelper::OnNearOomDetected, base::Unretained(this)));
}

void OomInterventionTabHelper::StopMonitoring() {
  subscription_.reset();
}

void OomInterventionTabHelper::OnNearOomDetected() {
  DCHECK(web_contents()->IsVisible());
  DCHECK(!near_oom_detected_time_);
  near_oom_detected_time_ = base::TimeTicks::Now();
  subscription_.reset();
}
