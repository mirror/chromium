// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/oom_page_load_metrics_observer_factory.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "ui/base/page_transition_types.h"

namespace {

class OomPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  explicit OomPageLoadMetricsObserver(
      OomPageLoadMetricsObserverFactory* factory)
      : factory_(factory) {}
  ~OomPageLoadMetricsObserver() override = default;

 private:
  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override {
    factory_->OnCommit(navigation_handle, source_id);
    return STOP_OBSERVING;
  }

  OomPageLoadMetricsObserverFactory* factory_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(OomPageLoadMetricsObserver);
};

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(OomPageLoadMetricsObserverFactory);

OomPageLoadMetricsObserverFactory::~OomPageLoadMetricsObserverFactory() =
    default;

std::unique_ptr<page_load_metrics::PageLoadMetricsObserver>
OomPageLoadMetricsObserverFactory::CreateObserver() {
  return std::make_unique<OomPageLoadMetricsObserver>(this);
}

void OomPageLoadMetricsObserverFactory::OnCommit(
    content::NavigationHandle* handle,
    ukm::SourceId source_id) {
  last_committed_source_id_ = source_id;
  last_committed_page_is_reload_ = ui::PageTransitionCoreTypeIs(
      handle->GetPageTransition(), ui::PAGE_TRANSITION_RELOAD);

  previous_committed_page_did_oom_ = last_committed_page_did_oom_;
  last_committed_page_did_oom_ = false;

  // Log the "reload chain" length. That is, log 0 for a single OOM. 1 for an
  // OOM followed by a reload that OOM'd, etc.
  if (!last_committed_page_is_reload_ || !previous_committed_page_did_oom_) {
    if (num_oom_reloads_ || previous_committed_page_did_oom_) {
      UMA_HISTOGRAM_EXACT_LINEAR("PageLoad.Clients.Memory.OomReloadChain",
                                 num_oom_reloads_, 50);
    }
    num_oom_reloads_ = 0;
  }
}

OomPageLoadMetricsObserverFactory::OomPageLoadMetricsObserverFactory(
    content::WebContents* web_contents)
    : oom_observer_(this) {
  auto* oom_reporter = OutOfMemoryReporter::FromWebContents(web_contents);
  DCHECK(oom_reporter);
  oom_observer_.Add(oom_reporter);
}

void OomPageLoadMetricsObserverFactory::OnForegroundOOMDetected(
    const GURL& url,
    ukm::SourceId source_id) {
  // The source ids should generally match, but since this notification is
  // coming async from the crash dump manager, weird things could happen.
  if (source_id != last_committed_source_id_)
    return;

  if (previous_committed_page_did_oom_ && last_committed_page_is_reload_)
    num_oom_reloads_++;

  last_committed_page_did_oom_ = true;
}
