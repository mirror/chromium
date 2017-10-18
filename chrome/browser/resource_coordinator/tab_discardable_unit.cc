// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_discardable_unit.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_importance_signals.h"
#include "url/gurl.h"

namespace resource_coordinator {

namespace {

constexpr base::TimeDelta kAudioProtectionTime =
    base::TimeDelta::FromSeconds(60);

#if !defined(OS_CHROMEOS)
constexpr base::TimeDelta kMinimumProtectionTime =
    base::TimeDelta::FromMinutes(10);
#endif

}  // namespace

TabDiscardableUnit::TabDiscardableUnit(TabManager* tab_manager,
                                       content::WebContents* web_contents,
                                       TabStripModel* tab_strip_model)
    : tab_manager_(tab_manager),
      web_contents_(web_contents),
      tab_strip_model_(tab_strip_model) {
  DCHECK(tab_manager_);
  DCHECK(web_contents_);
  DCHECK(tab_strip_model_);
}

TabDiscardableUnit::~TabDiscardableUnit() = default;

void TabDiscardableUnit::SetWebContents(content::WebContents* web_contents) {
  DCHECK(web_contents);
  web_contents_ = web_contents;
}

void TabDiscardableUnit::SetTabStripModel(TabStripModel* tab_strip_model) {
  tab_strip_model_ = tab_strip_model;
}

void TabDiscardableUnit::SetActive(bool active) {
  if (!active && active_)
    last_inactive_time_ = base::TimeTicks::Now();
  active_ = active;
}

void TabDiscardableUnit::SetRecentlyAudible(bool recently_audible) {
  if (recently_audible != recently_audible_) {
    recently_audible_ = recently_audible;
    recently_audible_time_ = base::TimeTicks::Now();
  }
}

content::RenderProcessHost* TabDiscardableUnit::GetRenderProcessHost() const {
  // maybe nullptr
  return web_contents_->GetMainFrame()->GetProcess();
}

DiscardableUnit::State TabDiscardableUnit::GetState() const {
  return state_;
}

bool TabDiscardableUnit::CanDiscard(SystemCondition system_condition) const {
#if defined(OS_CHROMEOS)
  if (/* TODO: advanced visibility check */)
    return false;
#else
  if (tab_strip_model_->GetActiveWebContents() == web_contents_)
    return false;
#endif  // defined(OS_CHROMEOS)

  // Do not discard tabs that don't have a valid URL (most probably they have
  // just been opened and discarding them would lose the URL).
  // TODO(georgesak): Look into a workaround to be able to kill the tab without
  // losing the pending navigation.
  if (!web_contents_->GetLastCommittedURL().is_valid() ||
      web_contents_->GetLastCommittedURL().is_empty()) {
    return false;
  }

  // Do not discard tabs in which the user has entered text in a form.
  if (web_contents_->GetPageImportanceSignals().had_form_interaction)
    return false;

  // Do not discard tabs that are playing either playing audio or accessing the
  // microphone or camera as it's too distruptive to the user experience. Note
  // that tabs that have recently stopped playing audio by at least
  // |kAudioProtectionTime| seconds are protected as well.
  if (IsMediaTab())
    return false;

  // Do not discard PDFs as they might contain entry that is not saved and they
  // don't remember their scrolling positions. See crbug.com/547286 and
  // crbug.com/65244.
  // TODO(georgesak): Remove this workaround when the bugs are fixed.
  if (web_contents_->GetContentsMimeType() == "application/pdf")
    return false;

// Do not discard a previously discarded tab if that's the desired behavior.
#if !defined(OS_CHROMEOS)
  if (discard_count_ > 0)
    return false;
#endif

// Do not discard a recently used tab.
#if !defined(OS_CHROMEOS)
  if (base::TimeTicks::Now() - last_inactive_time_ < kMinimumProtectionTime)
    return false;
#endif

  // Do not discard a tab that was explicitly disallowed to.
  if (!auto_discardable_)
    return false;

  return true;
}

void TabDiscardableUnit::Discard(State state,
                                 SystemCondition system_condition) {
  // Can't discard a tab that is already discarded.
  if (state_ == State::DISCARDED)
    return;

  tab_manager_->OnDiscardableUnitWillBeDiscarded(this);

  UMA_HISTOGRAM_BOOLEAN(
      "TabManager.Discarding.DiscardedTabHasBeforeUnloadHandler",
      web_contents_->NeedToFireBeforeUnload());

  content::WebContents* const old_contents = web_contents_;
  content::WebContents* const null_contents = content::WebContents::Create(
      content::WebContents::CreateParams(tab_strip_model_->profile()));
  // Copy over the state from the navigation controller to preserve the
  // back/forward history and to continue to display the correct title/favicon.
  //
  // Set |needs_reload| to false so that the tab is not automatically reloaded
  // when activated (otherwise, there would be an immediate reload when the
  // active tab in a non-visible window is discarded). TabManager will
  // explicitly reload the tab when it becomes the active tab in an active
  // window (ReloadWebContentsIfDiscarded).
  //
  // Note: It is important that |needs_reload| is false even when the discarded
  // tab is not active. Otherwise, it would get reloaded by
  // WebContentsImpl::WasShown() and by ReloadWebContentsIfDiscarded() when
  // activated.
  null_contents->GetController().CopyStateFrom(old_contents->GetController(),
                                               /* needs_reload */ false);

  // Make sure to persist the last active time property.
  null_contents->SetLastActiveTime(old_contents->GetLastActiveTime());

  // Copy over the TabManagerWebContentsData.
  // TODO(fdoray): Remove TabManagerWebContentsData.
  TabManager::WebContentsData::CopyState(old_contents, null_contents);

  // First try to fast-kill the process, if it's just running a single tab.
  bool fast_shutdown_success =
      GetRenderProcessHost()->FastShutdownIfPossible(1u, false);

#ifdef OS_CHROMEOS
  if (!fast_shutdown_success &&
      system_condition == SystemCondition::CRITICAL_MEMORY_PRESSURE) {
    content::RenderFrameHost* main_frame = old_contents->GetMainFrame();
    // We avoid fast shutdown on tabs with beforeunload handlers on the main
    // frame, as that is often an indication of unsaved user state.
    DCHECK(main_frame);
    if (!main_frame->GetSuddenTerminationDisablerState(
            blink::kBeforeUnloadHandler)) {
      fast_shutdown_success = GetRenderProcessHost()->FastShutdownIfPossible(
          1u, /* skip_unload_handlers */ true);
    }
    UMA_HISTOGRAM_BOOLEAN(
        "TabManager.Discarding.DiscardedTabCouldUnsafeFastShutdown",
        fast_shutdown_success);
  }
#endif
  UMA_HISTOGRAM_BOOLEAN("TabManager.Discarding.DiscardedTabCouldFastShutdown",
                        fast_shutdown_success);

  // Replace the discarded tab with the null version.
  const int index = tab_strip_model_->GetIndexOfWebContents(old_contents);
  DCHECK_NE(index, TabStripModel::kNoTab);
  tab_strip_model_->ReplaceWebContentsAt(index, null_contents);
  DCHECK_EQ(web_contents_, null_contents);

  // Record the discard in the TabManagerWebContentsData.
  // TODO(fdoray): Remove TabManagerWebContentsData.
  tab_manager_->GetWebContentsData(null_contents)->SetDiscardState(true);
  tab_manager_->GetWebContentsData(null_contents)->IncrementDiscardCount();

  ++discard_count_;

  // Discard the old tab's renderer.
  // TODO(jamescook): This breaks script connections with other tabs. Find a
  // different approach that doesn't do that, perhaps based on
  // RenderFrameProxyHosts.
  delete old_contents;
}

bool TabDiscardableUnit::IsMediaTab() const {
  if (web_contents_->WasRecentlyAudible())
    return true;

  scoped_refptr<MediaStreamCaptureIndicator> media_indicator =
      MediaCaptureDevicesDispatcher::GetInstance()
          ->GetMediaStreamCaptureIndicator();
  if (media_indicator->IsCapturingUserMedia(web_contents_) ||
      media_indicator->IsBeingMirrored(web_contents_)) {
    return true;
  }

  const base::TimeDelta recently_audible_time_delta =
      base::TimeTicks::Now() - recently_audible_time_;
  return recently_audible_time_delta < kAudioProtectionTime;
}

}  // namespace resource_coordinator
