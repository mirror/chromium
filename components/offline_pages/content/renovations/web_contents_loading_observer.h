// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CONTENT_RENOVATIONS_WEB_CONTENTS_LOADING_OBSERVER_H_
#define COMPONENTS_OFFLINE_PAGES_CONTENT_RENOVATIONS_WEB_CONTENTS_LOADING_OBSERVER_H_

#include <memory>

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_interface.h"
#include "services/resource_coordinator/public/interfaces/tab_signal.mojom.h"

namespace offline_pages {

class WebContentsLoadingObserver
    : public resource_coordinator::mojom::TabSignalObserver,
      public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsLoadingObserver> {
 public:
  ~WebContentsLoadingObserver() override;

  // TabSignalObserver implementation.
  void OnEventReceived(
      const resource_coordinator::CoordinationUnitID& id,
      resource_coordinator::mojom::TabEvent tab_event) override;

  // WebContentsObserver implementation.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInMainFrame() override;

 private:
  explicit WebContentsLoadingObserver(content::WebContents* web_contents);

  friend class content::WebContentsUserData<WebContentsLoadingObserver>;

  content::WebContents* web_contents_;

  mojo::Binding<resource_coordinator::mojom::TabSignalObserver> binding_;
  std::unique_ptr<resource_coordinator::ResourceCoordinatorInterface>
      resource_coordinator_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CONTENT_RENOVATIONS_WEB_CONTENTS_LOADING_OBSERVER_H_
