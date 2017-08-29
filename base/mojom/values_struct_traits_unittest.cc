// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/mojom/values_tester.mojom.h"
#include "base/test/gtest_util.h"
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

  void BounceDictionaryValue(Value in, BounceValueCallback callback) override {
    std::move(callback).Run(std::move(in));
  }

  void BounceListValue(Value in, BounceValueCallback callback) override {
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
  ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, BoolValue) {
  static constexpr bool kTestCases[] = {true, false};
  for (auto& test_case : kTestCases) {
    const Value in(test_case);
    Value out;
    ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
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
    ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
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
    ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
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
    ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
    EXPECT_EQ(in, out);
  }
}

TEST_F(ValuesStructTraitsTest, BinaryValue) {
  std::vector<char> kBinaryData = {'\x00', '\x80', '\xff', '\x7f', '\x01'};
  const Value in(std::move(kBinaryData));
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, DictionaryValue) {
  // Note: here and below, it would be nice to use an initializer list, but
  // move-only types and initializer lists don't mix. Initializer lists can't be
  // modified: thus it's not possible to move.
  std::vector<Value::DictStorage::value_type> storage;
  storage.emplace_back("null", std::make_unique<Value>());
  storage.emplace_back("bool", std::make_unique<Value>(false));
  storage.emplace_back("int", std::make_unique<Value>(0));
  storage.emplace_back("double", std::make_unique<Value>(0.0));
  storage.emplace_back("string", std::make_unique<Value>("0"));
  storage.emplace_back("binary",
                       std::make_unique<Value>(Value::BlobStorage({0})));
  storage.emplace_back("dictionary",
                       std::make_unique<Value>(Value::DictStorage()));
  storage.emplace_back("list", std::make_unique<Value>(Value::ListStorage()));
  const Value in(Value::DictStorage(std::move(storage), KEEP_LAST_OF_DUPES));
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
  EXPECT_EQ(in, out);

  ASSERT_TRUE(proxy_->BounceDictionaryValue(in.Clone(), &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, SerializeInvalidDictionaryValue) {
  const Value in;
  ASSERT_FALSE(in.is_dict());

  Value out;
  EXPECT_DCHECK_DEATH(proxy_->BounceDictionaryValue(in.Clone(), &out));
}

TEST_F(ValuesStructTraitsTest, ListValue) {
  Value::ListStorage storage;
  storage.emplace_back();
  storage.emplace_back(false);
  storage.emplace_back(0);
  storage.emplace_back(0.0);
  storage.emplace_back("0");
  storage.emplace_back(Value::BlobStorage({0}));
  storage.emplace_back(Value::DictStorage());
  storage.emplace_back(Value::ListStorage());
  const Value in(std::move(storage));
  Value out;
  ASSERT_TRUE(proxy_->BounceValue(in.Clone(), &out));
  EXPECT_EQ(in, out);

  ASSERT_TRUE(proxy_->BounceListValue(in.Clone(), &out));
  EXPECT_EQ(in, out);
}

TEST_F(ValuesStructTraitsTest, SerializeInvalidListValue) {
  const Value in;
  ASSERT_FALSE(in.is_dict());

  Value out;
  EXPECT_DCHECK_DEATH(proxy_->BounceListValue(in.Clone(), &out));
}

// A deeply nested Value should trigger a deserialization error.
TEST_F(ValuesStructTraitsTest, DeeplyNestedValue) {
  Value in;
  for (int i = 0; i < 100; ++i) {
    Value::ListStorage storage;
    storage.emplace_back(std::move(in));
    in = Value(std::move(storage));
  }
  Value out;
  // SerializationWarningObserverForTesting doesn't appear to have a
  // deserialization equivalent, so check for failure by verifying that the
  // sync call fails.
  EXPECT_FALSE(proxy_->BounceValue(in.Clone(), &out));
}

}  // namespace

}  // namespace base
