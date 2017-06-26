// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_H_

#include <string>

#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

// Objects implementing this class represent individual renovations
// that can be run in a page pre-snapshot.
class PageRenovation {
 public:
  // This method returns the renovation object should run in the
  // page obtained with this request.
  virtual bool ShouldRun(const SavePageRequest& request) const = 0;
  // This method returns a key that identifies the renovation
  // script. This key can be passed to the PageRenovationLoader.
  virtual std::string GetKey() const = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_H_
