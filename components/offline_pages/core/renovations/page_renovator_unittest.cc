// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/renovations/page_renovator.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/test/mock_callback.h"
#include "base/values.h"
#include "components/offline_pages/core/renovations/script_injector.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

// Provides a ScriptInjector instance that does nothing except set a
// flag and immediately call the callback upon Inject(). This flag can
// be queried through this class. We must have this creator class
// since PageRenovator takes ownership of its ScriptInjector.
class FakeScriptInjector {
 public:
  // Caller must ensure that this FakeScriptInjector outlives the
  // returned ScriptInjector instance.
  std::unique_ptr<ScriptInjector> Get();

  bool inject_called() const { return inject_called_; }

 private:
  // Class of object returned by Get().
  class ScriptInjectorImpl : public ScriptInjector {
   public:
    ScriptInjectorImpl(FakeScriptInjector* creator);
    ~ScriptInjectorImpl() override = default;

    void Inject(base::string16 script, ResultCallback callback) override;

   private:
    FakeScriptInjector* creator_;
  };

  bool inject_called_ = false;
};

std::unique_ptr<ScriptInjector> FakeScriptInjector::Get() {
  return base::MakeUnique<ScriptInjectorImpl>(this);
}

FakeScriptInjector::ScriptInjectorImpl::ScriptInjectorImpl(
    FakeScriptInjector* creator)
    : creator_(creator) {}

void FakeScriptInjector::ScriptInjectorImpl::Inject(base::string16 script,
                                                    ResultCallback callback) {
  if (callback)
    callback.Run(base::Value());
  creator_->inject_called_ = true;
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
  FakeScriptInjector fake_script_injector_;

  // PageRenovator under test.
  std::unique_ptr<PageRenovator> page_renovator_;
};

PageRenovatorTest::PageRenovatorTest() {
  // Set PageRenovationLoader to have empty script and Renovation list.
  page_renovation_loader_.SetSourceForTest(base::string16());
  page_renovation_loader_.SetRenovationsForTest(
      std::vector<std::unique_ptr<PageRenovation>>());

  page_renovator_ = base::MakeUnique<PageRenovator>(&page_renovation_loader_,
                                                    fake_script_injector_.Get(),
                                                    GURL("example.com"));
}

PageRenovatorTest::~PageRenovatorTest() {}

TEST_F(PageRenovatorTest, InjectsScript) {
  EXPECT_FALSE(fake_script_injector_.inject_called());
  page_renovator_->RunRenovations(base::Closure());
  EXPECT_TRUE(fake_script_injector_.inject_called());
}

TEST_F(PageRenovatorTest, CallsCallback) {
  base::MockCallback<base::Closure> mock_callback;
  EXPECT_CALL(mock_callback, Run()).Times(1);

  page_renovator_->RunRenovations(mock_callback.Get());
}

}  // namespace offline_pages
