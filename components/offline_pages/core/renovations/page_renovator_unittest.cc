// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/renovations/page_renovator.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/mock_callback.h"
#include "components/offline_pages/core/renovations/script_injector.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

class FakeScriptInjector : public ScriptInjector {
 public:
  FakeScriptInjector();

  void Inject(base::string16 script, ResultCallback callback) override;

  bool inject_called() const { return inject_called_; }

 private:
  base::TaskRunner* task_runner_;
  bool inject_called_;
};

FakeScriptInjector::FakeScriptInjector() : inject_called_(false) {
  DCHECK(task_runner_);
}

void FakeScriptInjector::Inject(base::string16 script,
                                ResultCallback callback) {
  callback.Run(nullptr);
  inject_called_ = true;
}

}  // namespace

class PageRenovatorTest : public testing::Test {
 public:
  PageRenovatorTest();
  ~PageRenovatorTest() override;

 protected:
  // Creates a PageRenovator with simple defaults for testing.
  void CreatePageRenovator(const GURL& url);

  PageRenovationLoader page_renovation_loader_;
  FakeScriptInjector script_injector_;

  // PageRenovator under test.
  std::unique_ptr<PageRenovator> page_renovator_;
};

PageRenovatorTest::PageRenovatorTest() {
  // Set PageRenovationLoader to have empty script and Renovation list.
  page_renovation_loader_.SetSourceForTest(base::UTF8ToUTF16(""));
  page_renovation_loader_.SetRenovationsForTest(
      std::vector<std::unique_ptr<PageRenovation>>());
}

PageRenovatorTest::~PageRenovatorTest() {}

void PageRenovatorTest::CreatePageRenovator(const GURL& url) {
  page_renovator_ = std::unique_ptr<PageRenovator>(
      new PageRenovator(&page_renovation_loader_, url, &script_injector_));
}

TEST_F(PageRenovatorTest, InjectsScript) {
  const GURL url("example.com");
  CreatePageRenovator(url);

  EXPECT_FALSE(script_injector_.inject_called());
  page_renovator_->RunRenovations(base::Closure());
  EXPECT_TRUE(script_injector_.inject_called());
}

TEST_F(PageRenovatorTest, CallsCallback) {
  const GURL url("example.com");
  CreatePageRenovator(url);

  base::MockCallback<base::Closure> mock_callback;
  EXPECT_CALL(mock_callback, Run()).Times(1);

  page_renovator_->RunRenovations(mock_callback.Get());
}

}  // namespace offline_pages
