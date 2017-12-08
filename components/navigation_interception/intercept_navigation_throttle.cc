// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/navigation_interception/intercept_navigation_throttle.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "components/navigation_interception/navigation_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"

using content::BrowserThread;

namespace navigation_interception {

const base::Feature kAsyncNavigationIntercept{"AsyncNavigationIntercept",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

InterceptNavigationThrottle::InterceptNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    CheckCallback should_ignore_callback)
    : content::NavigationThrottle(navigation_handle),
      should_ignore_callback_(should_ignore_callback),
      ui_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {}

InterceptNavigationThrottle::~InterceptNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::ElapsedTimer timer;

  auto result = CheckIfShouldIgnoreNavigation(false);
  if (navigation_handle()->IsInMainFrame()) {
    UMA_HISTOGRAM_COUNTS_10M("Navigation.Intercept.WillStart.Mainframe",
                             timer.Elapsed().InMicroseconds());
  } else {
    UMA_HISTOGRAM_COUNTS_10M("Navigation.Intercept.WillStart.Subframe",
                             timer.Elapsed().InMicroseconds());
  }
  return result;
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillRedirectRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return CheckIfShouldIgnoreNavigation(true);
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillFailRequest() {
  return WillFinish();
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillProcessResponse() {
  return WillFinish();
}

const char* InterceptNavigationThrottle::GetNameForLogging() {
  return "InterceptNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillFinish() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ready_to_finish_ = true;
  if (pending_check_)
    return content::NavigationThrottle::DEFER;
  return should_ignore_ ? content::NavigationThrottle::CANCEL_AND_IGNORE
                        : content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::CheckIfShouldIgnoreNavigation(bool is_redirect) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (should_ignore_) {
    DCHECK(is_redirect);
    DCHECK(!pending_check_);
    return content::NavigationThrottle::CANCEL_AND_IGNORE;
  }

  NavigationParams navigation_params(
      navigation_handle()->GetURL(), navigation_handle()->GetReferrer(),
      navigation_handle()->HasUserGesture(), navigation_handle()->IsPost(),
      navigation_handle()->GetPageTransition(), is_redirect,
      navigation_handle()->IsExternalProtocol(), true,
      navigation_handle()->GetBaseURLForDataURL());

  // Do not apply the optimization for POST navigations, to ensure we aren't
  // violating idempotency. Additionally, subframe navigation do not need to
  // consult with the activity manager, so let those through synchronously as
  // well.
  if (!navigation_handle()->IsInMainFrame() || navigation_handle()->IsPost() ||
      !base::FeatureList::IsEnabled(kAsyncNavigationIntercept)) {
    RunCheck(navigation_params);
    return should_ignore_ ? content::NavigationThrottle::CANCEL_AND_IGNORE
                          : content::NavigationThrottle::PROCEED;
  }
  pending_check_ = true;
  ui_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&InterceptNavigationThrottle::RunCheck,
                                weak_factory_.GetWeakPtr(), navigation_params));
  return content::NavigationThrottle::PROCEED;
}

void InterceptNavigationThrottle::RunCheck(const NavigationParams& params) {
  should_ignore_ = should_ignore_callback_.Run(
      navigation_handle()->GetWebContents(), params);
  pending_check_ = false;
  if (!ready_to_finish_)
    return;

  if (should_ignore_) {
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL_AND_IGNORE);
  } else {
    Resume();
  }
}

}  // namespace navigation_interception
