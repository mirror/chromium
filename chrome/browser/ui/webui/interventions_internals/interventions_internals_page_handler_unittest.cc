// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::unordered_map<std::string, bool> params_;

void MockMethod(const std::unordered_map<std::string, bool>& params) {
  params_ = params;
}

}  // namespace

class InterventionsInternalsPageHandlerTest : public testing::Test {
 public:
  InterventionsInternalsPageHandlerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {}

  ~InterventionsInternalsPageHandlerTest() override {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(InterventionsInternalsPageHandlerTest, FailingTest) {
  scoped_task_environment_.RunUntilIdle();

  mojom::InterventionsInternalsPageHandlerPtr pageHandlerPtr;
  auto request = mojo::MakeRequest(&pageHandlerPtr);

  std::unique_ptr<InterventionsInternalsPageHandler> pageHandler;
  pageHandler.reset(new InterventionsInternalsPageHandler(std::move(request)));

  pageHandler->GetPreviewsEnabled(base::BindOnce(*MockMethod));

  const unsigned long expected = 3;
  EXPECT_EQ(expected,params_.size());
}
