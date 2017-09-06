// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_snippets/ntp_snippets_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace {

const char kContentSuggestionsNotificationImpressions[] =
    "NewTabPage.ContentSuggestions.Notifications.Impressions";
const char kContentSuggestionsNotificationActions[] =
    "NewTabPage.ContentSuggestions.Notifications.Actions";
const char kContentSuggestionsNotificationOptOuts[] =
    "NewTabPage.ContentSuggestions.Notifications.OptOuts";
const char kContentSuggestionsNotificationAutoOptOuts[] =
    "NewTabPage.ContentSuggestions.Notifications.AutoOptOuts";

}  // namespace

void RecordContentSuggestionsNotificationImpression(
    ContentSuggestionsNotificationImpression what) {
  UMA_HISTOGRAM_ENUMERATION(kContentSuggestionsNotificationImpressions, what,
                            MAX_CONTENT_SUGGESTIONS_NOTIFICATION_IMPRESSION);
}

void RecordContentSuggestionsNotificationAction(
    ContentSuggestionsNotificationAction what) {
  UMA_HISTOGRAM_ENUMERATION(kContentSuggestionsNotificationActions, what,
                            MAX_CONTENT_SUGGESTIONS_NOTIFICATION_ACTION);
}

void RecordContentSuggestionsNotificationOptOut() {
  UMA_HISTOGRAM_BOOLEAN(kContentSuggestionsNotificationOptOuts, false);
}

void RecordContentSuggestionsNotificationAutoOptOut() {
  UMA_HISTOGRAM_ENUMERATION(kContentSuggestionsNotificationAutoOptOuts,
                            CONTENT_SUGGESTIONS_IMPLICIT,
                            MAX_CONTENT_SUGGESTIONS_NOTIFICATION_OPT_OUT);
}
