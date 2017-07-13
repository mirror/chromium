// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/mojo/values_tester.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

class ValuesTester : public mojom::ValuesTester {
 public:
  explicit ValuesTester(mojo::InterfaceRequest<mojom::ValuesTester> request)
      : binding_(this, std::move(request)) {}

  void BounceValue(Value in, BounceValueCallback callback) override {
    std::move(callback).Run(std::move(in));
  }

 private:
  mojo::Binding<mojom::ValuesTester> binding_;
};

class ValuesStructTraitsTest : public ::testing::Test {
 public:
  ValuesStructTraitsTest() : tester_(MakeRequest(&proxy_)) {}

 protected:
  MessageLoop message_loop_;
  mojom::ValuesTesterPtr proxy_;
  ValuesTester tester_;
};

TEST_F(ValuesStructTraitsTest, NullValue) {
  const Value in;
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in, &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, BoolValue) {
  static constexpr bool kTestCases[] = {true, false};
  for (auto& test_case : kTestCases) {
    const Value in(test_case);
    Value out;
    ASSERT_TRUE(proxy_->BounceValue(in, &out));
    EXPECT_EQ(in, out);
  }
}

TEST_F(ValuesStructTraitsTest, IntValue) {
  static constexpr int kTestCases[] = {0, -1, 1,
                                       std::numeric_limits<int>::min(),
                                       std::numeric_limits<int>::max()};
  for (auto& test_case : kTestCases) {
    const Value in(test_case);
    Value out;
    ASSERT_TRUE(proxy_->BounceValue(in, &out));
    EXPECT_EQ(in, out);
  }
}

TEST_F(ValuesStructTraitsTest, DoubleValue) {
  static constexpr double kTestCases[] = {-0.0,
                                          +0.0,
                                          -1.0,
                                          +1.0,
                                          std::numeric_limits<double>::min(),
                                          std::numeric_limits<double>::max()};
  for (auto& test_case : kTestCases) {
    const Value in(test_case);
    Value out;
    ASSERT_TRUE(proxy_->BounceValue(in, &out));
    EXPECT_EQ(in, out);
  }
}

TEST_F(ValuesStructTraitsTest, StringValue) {
  static constexpr const char* kTestCases[] = {
      "", "ascii",
      // 🎆: Unicode FIREWORKS
      "\xf0\x9f\x8e\x86",
  };
  for (auto* test_case : kTestCases) {
    const Value in(test_case);
    Value out;
    ASSERT_TRUE(proxy_->BounceValue(in, &out));
    EXPECT_EQ(in, out);
  }
}

TEST_F(ValuesStructTraitsTest, BinaryValue) {
  std::vector<char> kBinaryData = {'\x00', '\x80', '\xff', '\x7f', '\x01'};
  const Value in(std::move(kBinaryData));
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in, &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, DictionaryValue) {
  // Note: it would be nice to use an initializer list, but move-only types and
  // initializer lists don't mix. Initializer lists can't be modified: thus it's
  // not possible to move.
  std::vector<Value::DictStorage::value_type> storage;
  storage.emplace_back("null", MakeUnique<Value>());
  storage.emplace_back("bool", MakeUnique<Value>(false));
  storage.emplace_back("int", MakeUnique<Value>(0));
  storage.emplace_back("double", MakeUnique<Value>(0.0));
  storage.emplace_back("string", MakeUnique<Value>("0"));
  storage.emplace_back("binary", MakeUnique<Value>(Value::BlobStorage({0})));
  storage.emplace_back("dictionary", MakeUnique<Value>(Value::DictStorage()));
  storage.emplace_back("list", MakeUnique<Value>(Value::ListStorage()));
  const Value in(Value::DictStorage(std::move(storage), KEEP_LAST_OF_DUPES));
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in, &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, ListValue) {
  const Value in(Value::ListStorage({
      Value(), Value(false), Value(0), Value(0.0), Value("0"),
      Value(Value::BlobStorage({0})), Value(Value::DictStorage()),
      Value(Value::ListStorage()),
  }));
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in, &out));
  EXPECT_EQ(in, out);
}

// A deeply nested Value should trigger a deserialization error.
TEST_F(ValuesStructTraitsTest, DeeplyNestedValue) {
  base::Value in;
  for (int i = 0; i < 100; ++i)
    in = base::Value(Value::ListStorage({std::move(in)}));
  Value out;
  // SerializationWarningObserverForTesting doesn't appear to have a
  // deserialization equivalent, so check for failure by verifying that the
  // sync call fails.
  EXPECT_FALSE(proxy_->BounceValue(in, &out));
}

}  // namespace

}  // namespace base
