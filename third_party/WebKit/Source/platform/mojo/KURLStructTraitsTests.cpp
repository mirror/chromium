// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/mojo/KURLStructTraits.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(KURLStructTraitsTests, SerializeInvalid) {
  blink::KURL input;
  EXPECT_FALSE(input.IsValid());
  EXPECT_TRUE(input.IsEmpty());
  EXPECT_TRUE(input.GetString().IsNull());
  blink::KURL output;
  ASSERT_TRUE(url::mojom::blink::Url::Deserialize(
      url::mojom::blink::Url::Serialize(&input), &output));
  EXPECT_FALSE(output.IsValid());
  EXPECT_TRUE(output.IsEmpty());

  // The Mojo definition of a URL does not allow null strings. This means that
  // if a default URL is serialized/deserialized as so:
  //
  //   kurl -> serialize -> deserialize -> kurl'
  //
  // then kurl != kurl'
  //
  // If the Mojo definition is changed to allow this then GURL serialization
  // will break as this has no support for null url strings.
  EXPECT_FALSE(output.GetString().IsNull());
}

}  // namespace blink
