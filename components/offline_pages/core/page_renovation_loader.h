// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_LOADER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "components/offline_pages/core/page_renovation.h"

namespace offline_pages {

// Class for preparing page renovation scripts. Handles loading
// JavaScript from storage and creating script to run particular
// renovations.
class PageRenovationLoader {
 public:
  PageRenovationLoader();
  ~PageRenovationLoader();
  // Takes a list of renovation names and returns script to be run in page.
  base::string16 GetRenovationScript(
      const std::vector<std::string>& renovations);

 private:
  // Called to load JavaScript source from storage.
  void LoadSource();

  // Set combined_source_ for unit tests.
  void SetSourceForTest(base::string16 combined_source);

  // Set renovations_ for unit tests.
  void SetRenovationsForTest(
      std::vector<std::unique_ptr<PageRenovation>> renovations);

  // List of registered page renovations
  std::vector<std::unique_ptr<PageRenovation>> renovations_;

  // Whether JavaScript source has been loaded.
  bool is_loaded_;
  // Contains JavaScript source.
  base::string16 combined_source_;

  // PageRenovator instances need access to this internal data.
  friend class PageRenovator;

  // Friend classes for unit tests.
  friend class PageRenovationLoaderTest;
  friend class PageRenovatorTest;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PAGE_RENOVATION_LOADER_H_
