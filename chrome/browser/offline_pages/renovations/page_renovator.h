// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_RENOVATIONS_PAGE_RENOVATOR_H_
#define CHROME_BROWSER_OFFLINE_PAGES_RENOVATIONS_PAGE_RENOVATOR_H_

#include <memory>

#include "base/callback.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/offline_pages/content/renovations/script_injector.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/page_renovation_loader.h"
#include "content/public/browser/web_contents_observer.h"

namespace offline_pages {

// This class runs renovations in a page being offlined. Upon
// construction, it determines which renovations to run then
// registers an observer on the page's WebContents. This observer
// injects the renovations into the WebContent's main frame at the
// appropriate time.
class PageRenovator {
 public:
  PageRenovator(PageRenovationLoader* renovation_loader,
                const SavePageRequest& save_page_request,
                content::WebContents* web_contents);
  ~PageRenovator();

  // Set a callback to be called when scripts finish running in the
  // page, if ever. The callback will be called at most once. It will
  // not be called if the page fails to load or the PageRenovator is
  // destroyed before completion.
  void SetCompletionCallback(base::RepeatingClosure callback);

  // Set a different ScriptInjector to use (mainly for testing).
  void SetScriptInjector(std::unique_ptr<ScriptInjector> injector);

 private:
  // This method is used in the constructor to determine which
  // renovations to run and populate script_ with the renovations.
  void PrepareScript();

  // Passed as callback to ScriptInjector::AddScript. Calls user_callback_.
  void ScriptInjectorCallback();

  PageRenovationLoader* renovation_loader_;
  SavePageRequest save_page_request_;
  base::string16 script_;
  std::unique_ptr<ScriptInjector> injector_;
  base::RepeatingClosure user_callback_;
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_RENOVATIONS_PAGE_RENOVATOR_H_
