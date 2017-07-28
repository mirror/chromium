// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/dom_storage/dom_storage_map.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

namespace content {

class DOMStorageMapParamTest : public testing::Test,
                               public testing::WithParamInterface<bool> {
 public:
  DOMStorageMapParamTest() {}
  ~DOMStorageMapParamTest() override {}
};

INSTANTIATE_TEST_CASE_P(_, DOMStorageMapParamTest, ::testing::Bool());

TEST_P(DOMStorageMapParamTest, DOMStorageMapBasics) {
  const base::string16 kKey(ASCIIToUTF16("key"));
  const base::string16 kValue(ASCIIToUTF16("value"));
  const size_t kValueBytes = kValue.size() * sizeof(base::char16);
  const size_t kItemBytes =
      (kKey.size() + kValue.size()) * sizeof(base::char16);
  const base::string16 kKey2(ASCIIToUTF16("key2"));
  const size_t kKey2Bytes = kKey2.size() * sizeof(base::char16);
  const base::string16 kValue2(ASCIIToUTF16("value2"));
  const size_t kItem2Bytes =
      (kKey2.size() + kValue2.size()) * sizeof(base::char16);
  const size_t kQuota = 1024;  // 1K quota for this test.

  const bool has_only_keys = GetParam();
  scoped_refptr<DOMStorageMap> map(new DOMStorageMap(kQuota, has_only_keys));
  base::string16 old_value;
  base::NullableString16 old_nullable_value;
  DOMStorageValuesMap swap;
  scoped_refptr<DOMStorageMap> copy;

  // Check the behavior of an empty map.
  EXPECT_EQ(0u, map->Length());
  EXPECT_TRUE(map->Key(0).is_null());
  EXPECT_TRUE(map->Key(100).is_null());
  if (!has_only_keys)
    EXPECT_TRUE(map->GetItem(kKey).is_null());
  EXPECT_FALSE(map->RemoveItem(kKey));
  EXPECT_EQ(0u, map->bytes_used());
  copy = map->DeepCopy();
  EXPECT_EQ(0u, copy->Length());
  EXPECT_EQ(0u, copy->bytes_used());
  map->TakeValuesFrom(&swap);
  EXPECT_TRUE(swap.empty());

  // Check the behavior of a map containing some values.
  if (has_only_keys) {
    EXPECT_TRUE(map->SetItem(kKey, kValue));
  } else {
    EXPECT_TRUE(map->SetItem(kKey, kValue, &old_nullable_value));
    EXPECT_TRUE(old_nullable_value.is_null());
  }
  EXPECT_EQ(1u, map->Length());
  EXPECT_EQ(kKey, map->Key(0).string());
  EXPECT_TRUE(map->Key(1).is_null());
  if (!has_only_keys) {
    EXPECT_EQ(kValue, map->GetItem(kKey).string());
    EXPECT_TRUE(map->GetItem(kKey2).is_null());
  }
  EXPECT_EQ(kItemBytes, map->bytes_used());
  if (has_only_keys) {
    EXPECT_TRUE(map->RemoveItem(kKey));
  } else {
    EXPECT_TRUE(map->RemoveItem(kKey, &old_value));
    EXPECT_EQ(kValue, old_value);
  }
  old_value.clear();
  EXPECT_EQ(0u, map->bytes_used());

  EXPECT_TRUE(map->SetItem(kKey, kValue));
  EXPECT_TRUE(map->SetItem(kKey2, kValue));
  EXPECT_EQ(kItemBytes + kKey2Bytes + kValueBytes, map->bytes_used());
  if (has_only_keys) {
    EXPECT_TRUE(map->SetItem(kKey2, kValue2));
  } else {
    EXPECT_TRUE(map->SetItem(kKey2, kValue2, &old_nullable_value));
    EXPECT_EQ(kValue, old_nullable_value.string());
  }
  EXPECT_EQ(kItemBytes + kItem2Bytes, map->bytes_used());
  EXPECT_EQ(2u, map->Length());
  EXPECT_EQ(kKey, map->Key(0).string());
  EXPECT_EQ(kKey2, map->Key(1).string());
  EXPECT_EQ(kKey, map->Key(0).string());
  EXPECT_EQ(kItemBytes + kItem2Bytes, map->bytes_used());

  copy = map->DeepCopy();
  EXPECT_EQ(2u, copy->Length());
  if (!has_only_keys) {
    EXPECT_EQ(kValue, copy->GetItem(kKey).string());
    EXPECT_EQ(kValue2, copy->GetItem(kKey2).string());
  }
  EXPECT_EQ(kKey, copy->Key(0).string());
  EXPECT_EQ(kKey2, copy->Key(1).string());
  EXPECT_TRUE(copy->Key(2).is_null());
  EXPECT_EQ(kItemBytes + kItem2Bytes, copy->bytes_used());

  map->TakeValuesFrom(&swap);
  EXPECT_EQ(0u, map->Length());
  EXPECT_EQ(0u, map->bytes_used());
}

TEST_P(DOMStorageMapParamTest, EnforcesQuota) {
  const bool has_only_keys = GetParam();
  const base::string16 kKey = ASCIIToUTF16("test_key");
  const base::string16 kValue = ASCIIToUTF16("test_value");
  const base::string16 kKey2 = ASCIIToUTF16("test_key_2");
  const base::string16 kValueSize =
      base::SizeTToString16(kValue.length() * sizeof(base::char16));

  // A 50 byte quota is too small to hold both keys, so we
  // should see the DOMStorageMap enforcing it.
  const size_t kQuota = 50;

  scoped_refptr<DOMStorageMap> map(new DOMStorageMap(kQuota, has_only_keys));
  EXPECT_TRUE(map->SetItem(kKey, kValue));
  EXPECT_FALSE(map->SetItem(kKey2, kValue));
  if (has_only_keys) {
    EXPECT_EQ(kValueSize, map->values_[kKey].string());
  } else {
    EXPECT_EQ(kValue, map->values_[kKey].string());
  }
  EXPECT_EQ(1u, map->Length());

  EXPECT_TRUE(map->RemoveItem(kKey));
  EXPECT_EQ(0u, map->Length());
  EXPECT_TRUE(map->SetItem(kKey2, kValue));
  EXPECT_EQ(1u, map->Length());

  // Verify that the TakeValuesFrom method does not do quota checking.
  DOMStorageValuesMap swap;
  swap[kKey] = base::NullableString16(kValue, false);
  swap[kKey2] = base::NullableString16(kValue, false);
  map->TakeValuesFrom(&swap);
  EXPECT_GT(map->bytes_used(), kQuota);

  // When overbudget, a new value of greater size than the existing value can
  // not be set, but a new value of lesser or equal size can be set.
  EXPECT_TRUE(map->SetItem(kKey, kValue));
  EXPECT_FALSE(map->SetItem(kKey, base::string16(kValue + kValue)));
  EXPECT_TRUE(map->SetItem(kKey, base::string16()));
  if (has_only_keys) {
    EXPECT_EQ(ASCIIToUTF16("0"), map->values_[kKey].string());
  } else {
    EXPECT_EQ(base::string16(), map->values_[kKey].string());
  }
}

}  // namespace content
