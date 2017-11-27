// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_TEST_BROWSER_CONTEXT_H_
#define COMPONENTS_ARC_TEST_TEST_BROWSER_CONTEXT_H_

#include <memory>

#include "base/macros.h"
#include "content/public/test/test_browser_context.h"

class BrowserContextDependencyManager;
class TestingPrefServiceSimple;

namespace arc {

// A browser context for testing that can be used for getting objects
// through ArcBrowserContextKeyedServiceFactoryBase<>.
class TestBrowserContext : public content::TestBrowserContext {
 public:
  TestBrowserContext();
  ~TestBrowserContext() override;

 private:
  BrowserContextDependencyManager* const browser_context_dependency_manager_;
  std::unique_ptr<TestingPrefServiceSimple> prefs_;

  DISALLOW_COPY_AND_ASSIGN(TestBrowserContext);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_TEST_BROWSER_CONTEXT_H_
