// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#include <stdlib.h>

#include "v8.h"

#include "hashmap.h"

using namespace v8::internal;

static bool DefaultMatchFun(void* a, void* b) {
  return a == b;
}


class IntSet {
 public:
  IntSet() : map_(DefaultMatchFun)  {}

  void Insert(int x) {
    ASSERT(x != 0);  // 0 corresponds to (void*)NULL - illegal key value
    HashMap::Entry* p = map_.Lookup(reinterpret_cast<void*>(x), Hash(x), true);
    CHECK(p != NULL);  // insert is set!
    CHECK_EQ(reinterpret_cast<void*>(x), p->key);
    // we don't care about p->value
  }

  bool Present(int x) {
    HashMap::Entry* p = map_.Lookup(reinterpret_cast<void*>(x), Hash(x), false);
    if (p != NULL) {
      CHECK_EQ(reinterpret_cast<void*>(x), p->key);
    }
    return p != NULL;
  }

  void Clear() {
    map_.Clear();
  }

  uint32_t occupancy() const {
    uint32_t count = 0;
    for (HashMap::Entry* p = map_.Start(); p != NULL; p = map_.Next(p)) {
      count++;
    }
    CHECK_EQ(map_.occupancy(), static_cast<double>(count));
    return count;
  }

 private:
  HashMap map_;
  static uint32_t Hash(uint32_t key)  { return key * 23; }
};


static void TestSet() {
  IntSet set;
  CHECK_EQ(0, set.occupancy());

  set.Insert(1);
  set.Insert(2);
  set.Insert(3);
  CHECK_EQ(3, set.occupancy());

  set.Insert(2);
  set.Insert(3);
  CHECK_EQ(3, set.occupancy());

  CHECK(set.Present(1));
  CHECK(set.Present(2));
  CHECK(set.Present(3));
  CHECK(!set.Present(4));
  CHECK_EQ(3, set.occupancy());

  set.Clear();
  CHECK_EQ(0, set.occupancy());

  // Insert a long series of values.
  const int start = 453;
  const int factor = 13;
  const int offset = 7;
  const uint32_t n = 1000;

  int x = start;
  for (uint32_t i = 0; i < n; i++) {
    CHECK_EQ(i, static_cast<double>(set.occupancy()));
    set.Insert(x);
    x = x*factor + offset;
  }

  // Verify the same sequence of values.
  x = start;
  for (uint32_t i = 0; i < n; i++) {
    CHECK(set.Present(x));
    x = x*factor + offset;
  }

  CHECK_EQ(n, static_cast<double>(set.occupancy()));
}
