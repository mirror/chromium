// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_TRIGGERS_TRIGGER_CREATOR_H_
#define COMPONENTS_SAFE_BROWSING_TRIGGERS_TRIGGER_CREATOR_H_

class PrefService;

namespace content {
class WebContents;
}
namespace history {
class HistoryService;
}

namespace net {
class URLRequestContextGetter;
}

namespace safe_browsing {
class TriggerManager;

// Takes care of creation of individual triggers. This functionality lives in a
// separate class from TriggerManager to avoid circular dependencies.
// TriggerManager need not know about individual trigger classes, while the
// trigger classes needs to know about the TriggerManager in order to fire
// triggers.
class TriggerCreator {
 public:
  static void MaybeCreateTriggersForWebContents(
      content::WebContents* web_contents,
      TriggerManager* trigger_manager,
      PrefService* prefs,
      net::URLRequestContextGetter* request_context,
      history::HistoryService* history_service);
};

}  // namespace safe_browsing
#endif  // COMPONENTS_SAFE_BROWSING_TRIGGERS_TRIGGER_CREATOR_H_
