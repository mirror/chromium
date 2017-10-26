// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/presentation/PresentationAvailabilityState.h"

#include "modules/presentation/PresentationAvailabilityCallbacks.h"
#include "modules/presentation/PresentationAvailabilityObserver.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebCallbacks.h"
#include "public/platform/modules/presentation/WebPresentationError.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace blink {

using mojom::blink::ScreenAvailability;

class MockPresentationAvailabilityObserver
    : public PresentationAvailabilityObserver {
 public:
  explicit MockPresentationAvailabilityObserver(const Vector<KURL>& urls)
      : urls_(urls) {}
  ~MockPresentationAvailabilityObserver() override {}

  MOCK_METHOD1(AvailabilityChanged, void(ScreenAvailability availability));
  Vector<KURL> Urls() const override { return urls_; }

 private:
  const Vector<KURL> urls_;
};

class MockPresentationAvailabilityCallbacks
    : public PresentationAvailabilityCallbacks {
 public:
  MOCK_METHOD1(OnSuccess, void(bool value));
  MOCK_METHOD1(OnError, void(const blink::WebPresentationError&));
};

class TestPresentationAvailabilityState : public PresentationAvailabilityState {
 public:
  TestPresentationAvailabilityState()
      : PresentationAvailabilityState(nullptr) {}
  ~TestPresentationAvailabilityState() override {}

  MOCK_METHOD1(DoListenForScreenAvailability, void(const KURL&));
  MOCK_METHOD1(DoStopListeningForScreenAvailability, void(const KURL&));
};

class PresentationAvailabilityStateTest : public ::testing::Test {
 public:
  PresentationAvailabilityStateTest()
      : url1_(String("https://www.example.com/1.html")),
        url2_(KURL("https://www.example.com/2.html")),
        url3_(KURL("https://www.example.com/3.html")),
        url4_(KURL("https://www.example.com/4.html")),
        urls_({url1_, url2_, url3_, url4_}),
        observer_(urls_),
        mock_observer1_({url1_, url2_, url3_}),
        mock_observer2_({url2_, url3_, url4_}),
        mock_observer3_({url2_, url3_}),
        mock_observers_(
            {&mock_observer1_, &mock_observer2_, &mock_observer3_}) {}

  ~PresentationAvailabilityStateTest() override {}

  void ChangeURLState(const KURL& url, ScreenAvailability state) {
    switch (state) {
      case ScreenAvailability::AVAILABLE:
        state_.OnScreenAvailabilityUpdated(url, ScreenAvailability::AVAILABLE);
        break;
      case ScreenAvailability::SOURCE_NOT_SUPPORTED:
        state_.OnScreenAvailabilityUpdated(
            url, ScreenAvailability::SOURCE_NOT_SUPPORTED);
        break;
      case ScreenAvailability::UNAVAILABLE:
        state_.OnScreenAvailabilityUpdated(url,
                                           ScreenAvailability::UNAVAILABLE);
        break;
      case ScreenAvailability::DISABLED:
        state_.OnScreenAvailabilityUpdated(url, ScreenAvailability::DISABLED);
        break;
      case ScreenAvailability::UNKNOWN:
        break;
    }
  }

  // Tests that PresenationService is called for getAvailability(urls), after
  // |urls| change state to |states|. This function takes ownership of
  // |mock_callback|.
  void TestAddAvailabilityCallbacks(
      const Vector<KURL>& urls,
      const std::vector<ScreenAvailability>& states,
      MockPresentationAvailabilityCallbacks* mock_callback) {
    DCHECK_EQ(urls.size(), states.size());

    for (const auto& url : urls) {
      EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
      EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url)).Times(1);
    }

    state_.AddAvailabilityCallbacks(urls, base::WrapUnique(mock_callback));
    for (size_t i = 0; i < urls.size(); i++)
      ChangeURLState(urls[i], states[i]);
  }

 protected:
  const KURL url1_;
  const KURL url2_;
  const KURL url3_;
  const KURL url4_;
  const Vector<KURL> urls_;
  MockPresentationAvailabilityObserver observer_;
  MockPresentationAvailabilityObserver mock_observer1_;
  MockPresentationAvailabilityObserver mock_observer2_;
  MockPresentationAvailabilityObserver mock_observer3_;
  std::vector<MockPresentationAvailabilityObserver*> mock_observers_;
  TestPresentationAvailabilityState state_;
};

TEST_F(PresentationAvailabilityStateTest, TestListenForScreenAvailability) {
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url));
    EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url));
  }

  state_.AddAvailabilityCallbacks(
      urls_, std::make_unique<PresentationAvailabilityCallbacks>());
  state_.OnScreenAvailabilityUpdated(url1_, ScreenAvailability::AVAILABLE);

  for (const auto& url : urls_)
    EXPECT_CALL(state_, DoListenForScreenAvailability(url));

  state_.StartListeningForAvailability(&observer_);

  EXPECT_CALL(observer_, AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  state_.OnScreenAvailabilityUpdated(url1_, ScreenAvailability::UNAVAILABLE);
  EXPECT_CALL(observer_,
              AvailabilityChanged(ScreenAvailability::SOURCE_NOT_SUPPORTED));
  state_.OnScreenAvailabilityUpdated(url1_,
                                     ScreenAvailability::SOURCE_NOT_SUPPORTED);
  EXPECT_CALL(observer_, AvailabilityChanged(ScreenAvailability::AVAILABLE));
  state_.OnScreenAvailabilityUpdated(url1_, ScreenAvailability::AVAILABLE);
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url));
  }
  state_.StopListeningForAvailability(&observer_);

  // After StopListeningForAvailability(), |observer_| should no longer be
  // notified.
  EXPECT_CALL(observer_, AvailabilityChanged(ScreenAvailability::UNAVAILABLE))
      .Times(0);
  state_.OnScreenAvailabilityUpdated(url1_, ScreenAvailability::UNAVAILABLE);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksOneUrlNoAvailabilityChange) {
  auto* mock_callback =
      new testing::StrictMock<MockPresentationAvailabilityCallbacks>();

  EXPECT_CALL(state_, DoListenForScreenAvailability(url1_)).Times(1);

  state_.AddAvailabilityCallbacks(
      Vector<KURL>({url1_}),
      std::unique_ptr<PresentationAvailabilityCallbacks>(mock_callback));
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksOneUrlBecomesAvailable) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnSuccess(true));

  TestAddAvailabilityCallbacks({url1_}, {ScreenAvailability::AVAILABLE},
                               mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksOneUrlBecomesNotCompatible) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnSuccess(false));

  TestAddAvailabilityCallbacks(
      {url1_}, {ScreenAvailability::SOURCE_NOT_SUPPORTED}, mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksOneUrlBecomesUnavailable) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnSuccess(false));

  TestAddAvailabilityCallbacks({url1_}, {ScreenAvailability::UNAVAILABLE},
                               mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksOneUrlBecomesUnsupported) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnError(_));

  TestAddAvailabilityCallbacks({url1_}, {ScreenAvailability::DISABLED},
                               mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksMultipleUrlsAllBecomesAvailable) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnSuccess(true)).Times(1);

  TestAddAvailabilityCallbacks(
      {url1_, url2_},
      {ScreenAvailability::AVAILABLE, ScreenAvailability::AVAILABLE},
      mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksMultipleUrlsAllBecomesUnavailable) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnSuccess(false)).Times(1);

  TestAddAvailabilityCallbacks(
      {url1_, url2_},
      {ScreenAvailability::UNAVAILABLE, ScreenAvailability::UNAVAILABLE},
      mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksMultipleUrlsAllBecomesNotCompatible) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnSuccess(false)).Times(1);

  TestAddAvailabilityCallbacks({url1_, url2_},
                               {ScreenAvailability::SOURCE_NOT_SUPPORTED,
                                ScreenAvailability::SOURCE_NOT_SUPPORTED},
                               mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksMultipleUrlsAllBecomesUnsupported) {
  auto* mock_callback = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback, OnError(_)).Times(1);

  TestAddAvailabilityCallbacks(
      {url1_, url2_},
      {ScreenAvailability::DISABLED, ScreenAvailability::DISABLED},
      mock_callback);
}

TEST_F(PresentationAvailabilityStateTest,
       AddAvailabilityCallbacksReturnsDirectlyForAlreadyListeningUrls) {
  // First getAvailability() call.
  auto* mock_callback_1 = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback_1, OnSuccess(false)).Times(1);

  std::vector<ScreenAvailability> state_seq = {ScreenAvailability::UNAVAILABLE,
                                               ScreenAvailability::AVAILABLE,
                                               ScreenAvailability::UNAVAILABLE};
  TestAddAvailabilityCallbacks({url1_, url2_, url3_}, state_seq,
                               mock_callback_1);

  // Second getAvailability() call.
  for (const auto& url : mock_observer3_.Urls()) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
  }
  auto* mock_callback_2 = new MockPresentationAvailabilityCallbacks();
  EXPECT_CALL(*mock_callback_2, OnSuccess(true)).Times(1);

  state_.AddAvailabilityCallbacks(
      mock_observer3_.Urls(),
      std::unique_ptr<MockPresentationAvailabilityCallbacks>(mock_callback_2));
}

TEST_F(PresentationAvailabilityStateTest, StartListeningListenToEachURLOnce) {
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
  }

  for (auto* mock_observer : mock_observers_) {
    state_.AddAvailabilityCallbacks(
        mock_observer->Urls(),
        std::make_unique<PresentationAvailabilityCallbacks>());
    state_.StartListeningForAvailability(mock_observer);
  }
}

TEST_F(PresentationAvailabilityStateTest, StopListeningListenToEachURLOnce) {
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
    EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url)).Times(1);
  }

  EXPECT_CALL(mock_observer1_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  EXPECT_CALL(mock_observer2_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  EXPECT_CALL(mock_observer3_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));

  // Set up |availability_set_| and |listening_status_|
  for (auto* mock_observer : mock_observers_) {
    state_.AddAvailabilityCallbacks(
        mock_observer->Urls(),
        std::make_unique<PresentationAvailabilityCallbacks>());

    state_.StartListeningForAvailability(mock_observer);
  }

  // Clean up callbacks.
  ChangeURLState(url2_, ScreenAvailability::UNAVAILABLE);

  for (auto* mock_observer : mock_observers_)
    state_.StopListeningForAvailability(mock_observer);
}

TEST_F(PresentationAvailabilityStateTest,
       StopListeningDoesNotStopIfURLListenedByOthers) {
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
  }
  EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url1_)).Times(1);
  EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url2_)).Times(0);
  EXPECT_CALL(state_, DoStopListeningForScreenAvailability(url3_)).Times(0);

  // Set up |availability_set_| and |listening_status_|
  for (auto* mock_observer : mock_observers_) {
    state_.AddAvailabilityCallbacks(
        mock_observer->Urls(),
        std::make_unique<PresentationAvailabilityCallbacks>());
  }

  for (auto* mock_observer : mock_observers_)
    state_.StartListeningForAvailability(mock_observer);

  EXPECT_CALL(mock_observer1_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  EXPECT_CALL(mock_observer2_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  EXPECT_CALL(mock_observer3_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));

  // Clean up callbacks.
  ChangeURLState(url2_, ScreenAvailability::UNAVAILABLE);
  state_.StopListeningForAvailability(&mock_observer1_);
}

TEST_F(PresentationAvailabilityStateTest,
       OnScreenAvailabilityUpdatedInvokesAvailabilityChanged) {
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
  }
  EXPECT_CALL(mock_observer1_,
              AvailabilityChanged(ScreenAvailability::AVAILABLE));

  for (auto* mock_observer : mock_observers_) {
    state_.AddAvailabilityCallbacks(
        mock_observer->Urls(),
        std::make_unique<PresentationAvailabilityCallbacks>());
    state_.StartListeningForAvailability(mock_observer);
  }

  ChangeURLState(url1_, ScreenAvailability::AVAILABLE);

  EXPECT_CALL(mock_observer1_,
              AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  ChangeURLState(url1_, ScreenAvailability::UNAVAILABLE);

  EXPECT_CALL(mock_observer1_,
              AvailabilityChanged(ScreenAvailability::SOURCE_NOT_SUPPORTED));
  ChangeURLState(url1_, ScreenAvailability::SOURCE_NOT_SUPPORTED);
}

TEST_F(PresentationAvailabilityStateTest,
       OnScreenAvailabilityUpdatedInvokesMultipleAvailabilityChanged) {
  for (const auto& url : urls_) {
    EXPECT_CALL(state_, DoListenForScreenAvailability(url)).Times(1);
  }
  for (auto* mock_observer : mock_observers_) {
    EXPECT_CALL(*mock_observer,
                AvailabilityChanged(ScreenAvailability::AVAILABLE));
  }

  for (auto* mock_observer : mock_observers_) {
    state_.AddAvailabilityCallbacks(
        mock_observer->Urls(),
        std::make_unique<PresentationAvailabilityCallbacks>());
    state_.StartListeningForAvailability(mock_observer);
  }

  ChangeURLState(url2_, ScreenAvailability::AVAILABLE);

  for (auto* mock_observer : mock_observers_) {
    EXPECT_CALL(*mock_observer,
                AvailabilityChanged(ScreenAvailability::UNAVAILABLE));
  }
  ChangeURLState(url2_, ScreenAvailability::UNAVAILABLE);

  for (auto* mock_observer : mock_observers_) {
    EXPECT_CALL(*mock_observer,
                AvailabilityChanged(ScreenAvailability::SOURCE_NOT_SUPPORTED));
  }
  ChangeURLState(url2_, ScreenAvailability::SOURCE_NOT_SUPPORTED);
}

}  // namespace blink
