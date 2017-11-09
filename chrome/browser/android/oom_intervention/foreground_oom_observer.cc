// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/foreground_oom_observer.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ForegroundOomObserver);

ForegroundOomObserver::ForegroundOomObserver(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  OutOfMemoryReporter::FromWebContents(web_contents)->AddObserver(this);
}

void ForegroundOomObserver::WebContentsDestroyed() {
  OutOfMemoryReporter::FromWebContents(web_contents())->RemoveObserver(this);
}

void ForegroundOomObserver::OnForegroundOOMDetected(const GURL& url,
                                                    ukm::SourceId source_id) {
  LOG(ERROR) << "OOM: " << url;
}
