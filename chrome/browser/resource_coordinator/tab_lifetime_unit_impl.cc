// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifetime_unit_impl.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_observer.h"
#include "chrome/browser/resource_coordinator/time.h"
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

}  // namespace

#if !defined(OS_CHROMEOS)
constexpr base::TimeDelta TabLifetimeUnitImpl::kRecentUsageProtectionTime;
#endif

TabLifetimeUnitImpl::TabLifetimeUnitImpl(
    base::ObserverList<TabLifetimeObserver>* observers,
    content::WebContents* web_contents,
    TabStripModel* tab_strip_model)
    : content::WebContentsObserver(web_contents),
      observers_(observers),
      tab_strip_model_(tab_strip_model) {
  DCHECK(observers_);
  DCHECK(web_contents);
  DCHECK(tab_strip_model_);
  TabLifetimeUnit::SetForWebContents(web_contents, this);
}

TabLifetimeUnitImpl::~TabLifetimeUnitImpl() = default;

void TabLifetimeUnitImpl::SetRecentlyAudible(bool recently_audible) {
  if (recently_audible == recently_audible_)
    return;
  recently_audible_ = recently_audible;
  recently_audible_change_time_ = NowTicks();
}

void TabLifetimeUnitImpl::SetWebContents(content::WebContents* web_contents) {
  Observe(web_contents);
  TabLifetimeUnit::SetForWebContents(web_contents, this);
}

void TabLifetimeUnitImpl::SetFocused(bool focused) {
  const bool was_focused = last_focused_time_ == base::TimeTicks::Max();
  if (focused == was_focused)
    return;
  last_focused_time_ = focused ? base::TimeTicks::Max() : NowTicks();

  if (focused && state_ == State::DISCARDED) {
    web_contents()->GetController().SetNeedsReload();
    web_contents()->GetController().LoadIfNecessary();
    state_ = State::ALIVE;
    OnDiscardedStateChange();
  }
}

content::WebContents* TabLifetimeUnitImpl::GetWebContents() const {
  return web_contents();
}

bool TabLifetimeUnitImpl::IsMediaTab() const {
  if (web_contents()->WasRecentlyAudible())
    return true;

  scoped_refptr<MediaStreamCaptureIndicator> media_indicator =
      MediaCaptureDevicesDispatcher::GetInstance()
          ->GetMediaStreamCaptureIndicator();
  if (media_indicator->IsCapturingUserMedia(web_contents()) ||
      media_indicator->IsBeingMirrored(web_contents())) {
    return true;
  }

  if (recently_audible_change_time_.is_null())
    return false;

  const base::TimeDelta recently_audible_change_time_delta =
      NowTicks() - recently_audible_change_time_;
  return recently_audible_change_time_delta < kAudioProtectionTime;
}

bool TabLifetimeUnitImpl::IsAutoDiscardable() const {
  return auto_discardable_;
}

void TabLifetimeUnitImpl::SetAutoDiscardable(bool auto_discardable) {
  if (auto_discardable_ == auto_discardable)
    return;
  auto_discardable_ = auto_discardable;
  for (auto& observer : *observers_)
    observer.OnAutoDiscardableStateChange(web_contents(), auto_discardable_);
}

content::RenderProcessHost* TabLifetimeUnitImpl::GetRenderProcessHost() const {
  return web_contents()->GetMainFrame()->GetProcess();
}

TabLifetimeUnit* TabLifetimeUnitImpl::AsTabLifetimeUnit() {
  return this;
}

base::string16 TabLifetimeUnitImpl::GetTitle() const {
  return web_contents()->GetTitle();
}

base::ProcessHandle TabLifetimeUnitImpl::GetProcessHandle() const {
  return GetRenderProcessHost()->GetHandle();
}

LifetimeUnit::SortKey TabLifetimeUnitImpl::GetSortKey() const {
  return SortKey(last_focused_time_);
}

LifetimeUnit::State TabLifetimeUnitImpl::GetState() const {
  return state_;
}

int TabLifetimeUnitImpl::GetEstimatedMemoryFreedOnDiscardKB() const {
  std::unique_ptr<base::ProcessMetrics> process_metrics(
      base::ProcessMetrics::CreateProcessMetrics(GetProcessHandle()));
  base::WorkingSetKBytes mem_usage;
  process_metrics->GetWorkingSetKBytes(&mem_usage);
  return mem_usage.priv;
}

bool TabLifetimeUnitImpl::CanDiscard(DiscardCondition discard_condition) const {
  // Can't discard a tab that is already discarded.
  if (state_ == State::DISCARDED)
    return false;

  if (web_contents()->IsCrashed())
    return false;

#if defined(OS_CHROMEOS)
  // TODO(fdoray): Advanced occlusion check....
  if (tab_strip_model_->GetActiveWebContents() == web_contents())
    return false;
#else
  if (tab_strip_model_->GetActiveWebContents() == web_contents())
    return false;
#endif  // defined(OS_CHROMEOS)

  // Do not discard tabs that don't have a valid URL (most probably they have
  // just been opened and discarding them would lose the URL).
  // TODO(georgesak): Look into a workaround to be able to kill the tab without
  // losing the pending navigation.
  if (!web_contents()->GetLastCommittedURL().is_valid() ||
      web_contents()->GetLastCommittedURL().is_empty()) {
    return false;
  }

  // Do not discard tabs in which the user has entered text in a form.
  if (web_contents()->GetPageImportanceSignals().had_form_interaction)
    return false;

  // Do discard media tabs as it's too distruptive to the user experience.
  if (IsMediaTab())
    return false;

  // Do not discard PDFs as they might contain entry that is not saved and they
  // don't remember their scrolling positions. See crbug.com/547286 and
  // crbug.com/65244.
  // TODO(georgesak): Remove this workaround when the bugs are fixed.
  if (web_contents()->GetContentsMimeType() == "application/pdf")
    return false;

// On non-Chrome OS platforms, do not discard a previously discarded tab or a
// recently used tab. On Chrome OS, allow these tabs to be discarded as
// running out of memory causes a kernel panic.
#if !defined(OS_CHROMEOS)
  if (discard_count_ > 0)
    return false;

  // If the tab was never focused, use GetLastActiveTime() (equal to the tab
  // creation time) to determine when the tab was last used.
  base::TimeTicks last_focused_time = last_focused_time_.is_null()
                                          ? web_contents()->GetLastActiveTime()
                                          : last_focused_time_;

  if (NowTicks() - last_focused_time < kRecentUsageProtectionTime)
    return false;
#endif

  // Do not discard a tab that was explicitly disallowed to.
  if (!auto_discardable_)
    return false;

  return true;
}

bool TabLifetimeUnitImpl::Discard(DiscardCondition discard_condition) {
  if (state_ == State::DISCARDED)
    return false;

  UMA_HISTOGRAM_BOOLEAN(
      "TabManager.Discarding.DiscardedTabHasBeforeUnloadHandler",
      web_contents()->NeedToFireBeforeUnload());

  content::WebContents* const old_contents = web_contents();
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

  // Persist the last active time property.
  null_contents->SetLastActiveTime(old_contents->GetLastActiveTime());

  // First try to fast-kill the process, if it's just running a single tab.
  bool fast_shutdown_success =
      GetRenderProcessHost()->FastShutdownIfPossible(1u, false);

#if defined(OS_CHROMEOS)
  if (!fast_shutdown_success &&
      discard_condition == DiscardCondition::kUrgent) {
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
  DCHECK_EQ(web_contents(), null_contents);

  // Discard the old tab's renderer.
  // TODO(jamescook): This breaks script connections with other tabs. Find a
  // different approach that doesn't do that, perhaps based on
  // RenderFrameProxyHosts.
  delete old_contents;

  state_ = State::DISCARDED;
  ++discard_count_;
  OnDiscardedStateChange();

  return true;
}

void TabLifetimeUnitImpl::OnDiscardedStateChange() {
  const bool is_discarded = state_ == State::DISCARDED;
  for (auto& observer : *observers_)
    observer.OnDiscardedStateChange(web_contents(), is_discarded);
}

void TabLifetimeUnitImpl::DidStartLoading() {
  if (state_ == State::DISCARDED) {
    state_ = State::ALIVE;
    OnDiscardedStateChange();
  }
}

}  // namespace resource_coordinator
