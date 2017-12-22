// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_UNITTEST_COMMON_H_
#define SERVICES_PREFERENCES_UNITTEST_COMMON_H_

#include "components/prefs/pref_store.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace prefs {

constexpr int kInitialValue = 1;
constexpr char kKey[] = "path.to.key";
constexpr char kOtherKey[] = "path.to.other_key";
constexpr char kDictionaryKey[] = "a.dictionary.pref";
constexpr char kInitialKey[] = "initial_key";
constexpr char kOtherInitialKey[] = "other_initial_key";

class PrefStoreObserverMock : public PrefStore::Observer {
 public:
  PrefStoreObserverMock();
  ~PrefStoreObserverMock();
  MOCK_METHOD1(OnPrefValueChanged, void(const std::string&));
  MOCK_METHOD1(OnInitializationCompleted, void(bool));
};

void ExpectPrefChange(PrefStore* pref_store, base::StringPiece key) {
  PrefStoreObserverMock observer;
  pref_store->AddObserver(&observer);
  base::RunLoop run_loop;
  EXPECT_CALL(observer, OnPrefValueChanged(key.as_string()))
      .WillOnce(testing::WithoutArgs(
          testing::Invoke([&run_loop]() { run_loop.Quit(); })));
  run_loop.Run();
  pref_store->RemoveObserver(&observer);
}

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_UNITTEST_COMMON_H_
