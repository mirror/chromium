// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/predictor_tab_helper.h"

#include "chrome/browser/net/predictor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_handle.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif  // defined(OS_CHROMEOS)

DEFINE_WEB_CONTENTS_USER_DATA_KEY(chrome_browser_net::PredictorTabHelper);

namespace chrome_browser_net {

PredictorTabHelper::PredictorTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

PredictorTabHelper::~PredictorTabHelper() {
}

void PredictorTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!base::FeatureList::IsEnabled(features::kPreconnectMore) &&
      navigation_handle->IsRendererInitiated()) {
    return;
  }
  // Subframe navigations are handled in WitnessURLRequest.
  if (!navigation_handle->IsInMainFrame())
    return;
  PreconnectUrl(navigation_handle->GetURL());
}

void PredictorTabHelper::DocumentOnLoadCompletedInMainFrame() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  Predictor* predictor = profile->GetNetworkPredictor();
#if defined(OS_CHROMEOS)
  if (chromeos::ProfileHelper::IsSigninProfile(profile))
    return;
#endif
  if (predictor)
    predictor->SaveStateForNextStartup();
}

void PredictorTabHelper::PreconnectUrl(const GURL& url) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  Predictor* predictor(profile->GetNetworkPredictor());
  if (predictor && url.SchemeIsHTTPOrHTTPS())
    predictor->PreconnectUrlAndSubresources(url, GURL());
}

}  // namespace chrome_browser_net
