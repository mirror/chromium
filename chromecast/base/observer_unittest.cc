// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <functional>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chromecast/base/observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromecast {

class ObserverTest : public ::testing::Test {
 protected:
  ObserverTest() : message_loop_(new base::MessageLoop()) {}

  const std::unique_ptr<base::MessageLoop> message_loop_;
};

struct NoDefaultConstructor {
  NoDefaultConstructor(int v) : value(v) {}

  int value;
};

void RunCallback(std::function<void()> callback) {
  callback();
}

TEST_F(ObserverTest, SimpleValue) {
  Observable<int> original(0);
  Observer<int> observer = original.Observe();

  EXPECT_EQ(0, observer.GetValue());

  original.SetValue(1);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, observer.GetValue());
}

TEST_F(ObserverTest, MultipleObservers) {
  Observable<int> original(0);
  Observer<int> observer1 = original.Observe();
  Observer<int> observer2 = observer1;

  EXPECT_EQ(0, observer1.GetValue());
  EXPECT_EQ(0, observer2.GetValue());

  original.SetValue(1);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, observer1.GetValue());
  EXPECT_EQ(1, observer2.GetValue());
}

TEST_F(ObserverTest, NoDefaultConstructor) {
  Observable<NoDefaultConstructor> original(0);
  Observer<NoDefaultConstructor> observer = original.Observe();

  EXPECT_EQ(0, observer.GetValue().value);

  original.SetValue(1);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, observer.GetValue().value);
}

TEST_F(ObserverTest, NoMissingEvents) {
  Observable<int> original(0);
  Observer<int> observer = original.Observe();
  original.SetValue(1);

  std::vector<int> event_values;
  std::function<void()> callback = [&]() {
    event_values.push_back(observer.GetValue());
  };
  observer.SetOnUpdateCallback(base::BindRepeating(&RunCallback, callback));

  EXPECT_EQ(0, observer.GetValue());

  original.SetValue(2);
  base::RunLoop().RunUntilIdle();
  original.SetValue(3);
  original.SetValue(4);
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(4u, event_values.size());
  EXPECT_EQ(1, event_values[0]);
  EXPECT_EQ(2, event_values[1]);
  EXPECT_EQ(3, event_values[2]);
  EXPECT_EQ(4, event_values[3]);

  EXPECT_EQ(4, observer.GetValue());
}

TEST_F(ObserverTest, NoExtraEvents) {
  Observable<int> original(0);
  Observer<int> observer1 = original.Observe();
  std::vector<int> event_values1;
  std::function<void()> callback1 = [&]() {
    event_values1.push_back(observer1.GetValue());
  };
  observer1.SetOnUpdateCallback(base::BindRepeating(&RunCallback, callback1));

  original.SetValue(1);
  // The SetValue event hasn't propagated yet.
  EXPECT_EQ(0, observer1.GetValue());

  Observer<int> observer2 = original.Observe();
  std::vector<int> event_values2;
  std::function<void()> callback2 = [&]() {
    event_values2.push_back(observer2.GetValue());
  };
  observer2.SetOnUpdateCallback(base::BindRepeating(&RunCallback, callback2));

  // New Observers get the up-to-date value.
  EXPECT_EQ(1, observer2.GetValue());

  original.SetValue(2);

  // Events still haven't propagated.
  EXPECT_EQ(0, observer1.GetValue());
  EXPECT_EQ(1, observer2.GetValue());

  base::RunLoop().RunUntilIdle();

  // All events should have arrived, and the observers have the updated value.
  ASSERT_EQ(2u, event_values1.size());
  EXPECT_EQ(1, event_values1[0]);
  EXPECT_EQ(2, event_values1[1]);

  // observer2 already knew about the value '1', so it didn't get that event.
  ASSERT_EQ(1u, event_values2.size());
  EXPECT_EQ(2, event_values2[0]);

  EXPECT_EQ(2, observer1.GetValue());
  EXPECT_EQ(2, observer2.GetValue());
}

TEST_F(ObserverTest, Lifetime) {
  std::unique_ptr<Observable<int>> original(new Observable<int>(0));
  Observer<int> observer1 = original->Observe();

  EXPECT_EQ(0, observer1.GetValue());

  original->SetValue(1);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, observer1.GetValue());

  original.reset();

  Observer<int> observer2 = observer1;
  EXPECT_EQ(1, observer2.GetValue());
}

}  // chromecast
