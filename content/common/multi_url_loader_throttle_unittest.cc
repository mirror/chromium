// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/multi_url_loader_throttle.h"

#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response_info.h"
#include "content/public/common/url_loader_throttle.h"
#include "net/url_request/redirect_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;

namespace content {

namespace {

ACTION_P2(CancelWithError, delegate, error_code) {
  delegate->CancelWithError(error_code);
}

ACTION(Defer) {
  *arg1 = true;
}

class MockURLLoaderThrottleDelegate : public URLLoaderThrottle::Delegate {
 public:
  MockURLLoaderThrottleDelegate() = default;
  ~MockURLLoaderThrottleDelegate() = default;

  // URLLoaderThrottle::Delegate implementation.
  MOCK_METHOD1(CancelWithError, void(int));
  MOCK_METHOD0(Resume, void(void));
};

class MockURLLoaderThrottle : public URLLoaderThrottle {
 public:
  MockURLLoaderThrottle() = default;
  ~MockURLLoaderThrottle() = default;

  // URLLoaderThrottle implementation.
  MOCK_METHOD2(WillStartRequest, void(const ResourceRequest&, bool*));
  MOCK_METHOD2(WillRedirectRequest, void(const net::RedirectInfo&, bool*));
  MOCK_METHOD2(WillProcessResponse, void(const ResourceResponseInfo&, bool*));

  URLLoaderThrottle::Delegate* delegate() { return delegate_; }
};

TEST(MultiURLLoaderThrottle, ThrottlePassthrough) {
  testing::InSequence sequence;

  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());

  auto* throttle1 = static_cast<MockURLLoaderThrottle*>(throttles[0].get());
  auto* throttle2 = static_cast<MockURLLoaderThrottle*>(throttles[1].get());

  MultiURLLoaderThrottle multi_throttle(std::move(throttles));

  EXPECT_CALL(*throttle1, WillStartRequest(_, _)).Times(1);
  EXPECT_CALL(*throttle2, WillStartRequest(_, _)).Times(1);
  EXPECT_CALL(*throttle1, WillRedirectRequest(_, _)).Times(1);
  EXPECT_CALL(*throttle2, WillRedirectRequest(_, _)).Times(1);
  EXPECT_CALL(*throttle1, WillProcessResponse(_, _)).Times(1);
  EXPECT_CALL(*throttle2, WillProcessResponse(_, _)).Times(1);

  bool defer = false;
  multi_throttle.WillStartRequest(ResourceRequest(), &defer);
  multi_throttle.WillRedirectRequest(net::RedirectInfo(), &defer);
  multi_throttle.WillProcessResponse(ResourceResponseInfo(), &defer);
}

TEST(MultiURLLoaderThrottle, CancelStopsOtherApiCalls) {
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());

  auto* throttle1 = static_cast<MockURLLoaderThrottle*>(throttles[0].get());
  auto* throttle2 = static_cast<MockURLLoaderThrottle*>(throttles[1].get());

  MultiURLLoaderThrottle multi_throttle(std::move(throttles));
  MockURLLoaderThrottleDelegate delegate;
  multi_throttle.set_delegate(&delegate);

  EXPECT_CALL(*throttle1, WillStartRequest(_, _))
      .Times(1)
      .WillOnce(CancelWithError(throttle1->delegate(), 0));
  EXPECT_CALL(*throttle2, WillStartRequest(_, _)).Times(0);
  EXPECT_CALL(*throttle1, WillRedirectRequest(_, _)).Times(0);
  EXPECT_CALL(*throttle2, WillRedirectRequest(_, _)).Times(0);
  EXPECT_CALL(*throttle1, WillProcessResponse(_, _)).Times(0);
  EXPECT_CALL(*throttle2, WillProcessResponse(_, _)).Times(0);
  EXPECT_CALL(delegate, CancelWithError(0)).Times(1);

  bool defer = false;
  multi_throttle.WillStartRequest(ResourceRequest(), &defer);
  multi_throttle.WillRedirectRequest(net::RedirectInfo(), &defer);
  multi_throttle.WillProcessResponse(ResourceResponseInfo(), &defer);
}

TEST(MultiURLLoaderThrottle, SingleDeferOnMultiThrottle) {
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());

  auto* throttle1 = static_cast<MockURLLoaderThrottle*>(throttles[0].get());
  auto* throttle2 = static_cast<MockURLLoaderThrottle*>(throttles[1].get());

  MultiURLLoaderThrottle multi_throttle(std::move(throttles));
  MockURLLoaderThrottleDelegate delegate;
  multi_throttle.set_delegate(&delegate);

  {
    EXPECT_CALL(*throttle1, WillStartRequest(_, _)).Times(1).WillOnce(Defer());
    EXPECT_CALL(*throttle2, WillStartRequest(_, _)).Times(1);

    bool defer = false;
    multi_throttle.WillStartRequest(ResourceRequest(), &defer);
    EXPECT_TRUE(defer);
  }

  {
    EXPECT_CALL(delegate, Resume()).Times(1);
    throttle1->delegate()->Resume();
  }
}

TEST(MultiURLLoaderThrottle, MultiDeferOnMultiThrottle) {
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());

  auto* throttle1 = static_cast<MockURLLoaderThrottle*>(throttles[0].get());
  auto* throttle2 = static_cast<MockURLLoaderThrottle*>(throttles[1].get());

  MultiURLLoaderThrottle multi_throttle(std::move(throttles));
  MockURLLoaderThrottleDelegate delegate;
  multi_throttle.set_delegate(&delegate);

  {
    EXPECT_CALL(*throttle1, WillStartRequest(_, _)).Times(1).WillOnce(Defer());
    EXPECT_CALL(*throttle2, WillStartRequest(_, _)).Times(1).WillOnce(Defer());

    bool defer = false;
    multi_throttle.WillStartRequest(ResourceRequest(), &defer);
    EXPECT_TRUE(defer);
  }

  {
    EXPECT_CALL(delegate, Resume()).Times(0);
    throttle1->delegate()->Resume();
  }

  {
    EXPECT_CALL(delegate, Resume()).Times(1);
    throttle2->delegate()->Resume();
  }
}

TEST(MultiURLLoaderThrottle, ResumeNoOpIfNotDeferred) {
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles;
  throttles.push_back(base::MakeUnique<MockURLLoaderThrottle>());

  auto* throttle1 = static_cast<MockURLLoaderThrottle*>(throttles[0].get());

  MultiURLLoaderThrottle multi_throttle(std::move(throttles));
  MockURLLoaderThrottleDelegate delegate;
  multi_throttle.set_delegate(&delegate);

  EXPECT_CALL(*throttle1, WillStartRequest(_, _)).Times(1);
  EXPECT_CALL(delegate, Resume()).Times(0);

  throttle1->delegate()->Resume();

  bool defer = false;
  multi_throttle.WillStartRequest(ResourceRequest(), &defer);

  throttle1->delegate()->Resume();
}

}  // namespace

}  // namespace content
