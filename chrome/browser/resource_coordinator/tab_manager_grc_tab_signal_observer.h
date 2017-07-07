// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_GRC_TAB_SIGNAL_OBSERVER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_GRC_TAB_SIGNAL_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/interfaces/tab_signal_generator.mojom.h"

namespace content {
class WebContents;
}

namespace resource_coordinator {

class TabManager::GRCTabSignalObserver : public mojom::TabSignalObserver {
 public:
  GRCTabSignalObserver();
  ~GRCTabSignalObserver() override;

  // mojom::TabSignalObserver implementation.
  void SendProperty(const CoordinationUnitID& cu_id,
                    mojom::PropertyType property_type,
                    std::unique_ptr<base::Value> value) override {}
  void SendEvent(const CoordinationUnitID& cu_id,
                 mojom::TabEvent event) override;

  void AssociateCoordinationUnitIDWithWebContents(
      content::WebContents* web_contents,
      const CoordinationUnitID& cu_id);

 private:
  mojo::Binding<mojom::TabSignalObserver> binding_;
  std::map<CoordinationUnitID, content::WebContents*> tabs_;
  DISALLOW_COPY_AND_ASSIGN(GRCTabSignalObserver);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_GRC_TAB_SIGNAL_OBSERVER_H_
