// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/mojo/KURLStructTraits.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(KURLStructTraitsTests, SerializeEmpty) {
  blink::KURL input("");
  EXPECT_FALSE(input.IsValid());
  EXPECT_TRUE(input.IsEmpty());
  EXPECT_FALSE(input.GetString().IsNull());
  blink::KURL output;
  ASSERT_TRUE(url::mojom::blink::Url::Deserialize(
      url::mojom::blink::Url::Serialize(&input), &output));
  EXPECT_FALSE(output.IsValid());
  EXPECT_TRUE(output.IsEmpty());
  EXPECT_FALSE(output.GetString().IsNull());
}

TEST(KURLStructTraitsTests, SerializeTooLong) {
  WTF::String str("https://www.long-url.com:42/");
  str.append(std::string(url::kMaxURLChars + 10, 'x').c_str());
  blink::KURL input(str);
  EXPECT_TRUE(input.IsValid());
  blink::KURL output;
  ASSERT_TRUE(url::mojom::blink::Url::Deserialize(
      url::mojom::blink::Url::Serialize(&input), &output));
  // URLs which exceed maximum length (url::kMaxURLChars) are quietly truncated.
  EXPECT_FALSE(output.IsValid());
  EXPECT_TRUE(output.IsEmpty());
  EXPECT_FALSE(output.IsNull());
}

TEST(KURLStructTraitsTests, SerializeValid) {
  blink::KURL input("https://www.google.com:42");
  blink::KURL output;
  ASSERT_TRUE(url::mojom::blink::Url::Deserialize(
      url::mojom::blink::Url::Serialize(&input), &output));
  EXPECT_EQ(input.IsValid(), output.IsValid());
  EXPECT_EQ(input.IsEmpty(), output.IsEmpty());
  EXPECT_EQ(input.GetString(), output.GetString());
  EXPECT_EQ(input.HasPort(), output.HasPort());
  EXPECT_EQ(input.Port(), output.Port());
  EXPECT_EQ(input.Protocol(), output.Protocol());
  EXPECT_EQ(input.Host(), output.Host());
}

}  // namespace blink
