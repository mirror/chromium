// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/observer_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class ObserverListTest : public testing::Test {
};

class Foo {
 public:
  virtual void Observe(int x) = 0;
  virtual ~Foo() {}
};

class Adder : public Foo {
 public:
  Adder(int scaler) : total(0), scaler_(scaler) {}
  virtual void Observe(int x) {
    total += x * scaler_;
  }
  virtual ~Adder() { }
  int total;
 private:
  int scaler_;
};

class Disrupter : public Foo {
 public:
  Disrupter(ObserverList<Foo>& list, Foo* doomed) : list_(list), doomed_(doomed) {
  }
  virtual ~Disrupter() { }
  virtual void Observe(int x) {
    list_.RemoveObserver(doomed_);
  }
 private:
  ObserverList<Foo>& list_;
  Foo* doomed_;
};

class AddInObserve : public Foo {
 public:
  AddInObserve(ObserverList<Foo>* observer_list)
      : added(false),
        observer_list(observer_list),
        adder(1) {
  }
  virtual void Observe(int x) {
    if (!added) {
      added = true;
      observer_list->AddObserver(&adder);
    }
  }

  bool added;
  ObserverList<Foo>* observer_list;
  Adder adder;
};



}  // namespace

TEST(ObserverListTest, BasicTest) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter evil(observer_list, &c);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  FOR_EACH_OBSERVER(Foo, observer_list, Observe(10));

  observer_list.AddObserver(&evil);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  FOR_EACH_OBSERVER(Foo, observer_list, Observe(10));

  EXPECT_EQ(a.total, 20);
  EXPECT_EQ(b.total, -20);
  EXPECT_EQ(c.total, 0);
  EXPECT_EQ(d.total, -10);
}


TEST(ObserverListTest, Existing) {
  ObserverList<Foo> observer_list(ObserverList<Foo>::NOTIFY_EXISTING_ONLY);
  Adder a(1);
  AddInObserve b(&observer_list);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  FOR_EACH_OBSERVER(Foo, observer_list, Observe(1));

  EXPECT_TRUE(b.added);
  // B's adder should not have been notified because it was added during
  // notificaiton.
  EXPECT_EQ(0, b.adder.total);

  // Notify again to make sure b's adder is notified.
  FOR_EACH_OBSERVER(Foo, observer_list, Observe(1));
  EXPECT_EQ(1, b.adder.total);
}
