// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/parser/TextResourceDecoder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(TextResourceDecoderTest, BasicUTF16) {
  std::unique_ptr<TextResourceDecoder> decoder =
      TextResourceDecoder::Create(TextResourceDecoderOptions(
          TextResourceDecoderOptions::kPlainTextContent));
  WTF::String decoded;

  const unsigned char kFooLE[] = {0xff, 0xfe, 0x66, 0x00,
                                  0x6f, 0x00, 0x6f, 0x00};
  decoded =
      decoder->Decode(reinterpret_cast<const char*>(kFooLE), sizeof(kFooLE));
  decoded = decoded + decoder->Flush();
  EXPECT_EQ("foo", decoded);

  decoder = TextResourceDecoder::Create(TextResourceDecoderOptions(
      TextResourceDecoderOptions::kPlainTextContent));
  const unsigned char kFooBE[] = {0xfe, 0xff, 0x00, 0x66,
                                  0x00, 0x6f, 0x00, 0x6f};
  decoded =
      decoder->Decode(reinterpret_cast<const char*>(kFooBE), sizeof(kFooBE));
  decoded = decoded + decoder->Flush();
  EXPECT_EQ("foo", decoded);
}

TEST(TextResourceDecoderTest, UTF16Pieces) {
  std::unique_ptr<TextResourceDecoder> decoder =
      TextResourceDecoder::Create(TextResourceDecoderOptions(
          TextResourceDecoderOptions::kPlainTextContent));

  WTF::String decoded;
  const unsigned char kFoo[] = {0xff, 0xfe, 0x66, 0x00, 0x6f, 0x00, 0x6f, 0x00};
  for (char c : kFoo) {
    String chunk = decoder->Decode(reinterpret_cast<const char*>(&c), 1);
    printf("%d ", chunk.length());
    if (chunk.IsNull())
      printf("Null\n");
    else if (chunk.Is8Bit())
      printf("8Bit\n");
    else
      printf("16Bit\n");
    decoded = decoded + chunk;
  }
  decoded = decoded + decoder->Flush();
  EXPECT_EQ("foo", decoded);
}

TEST(TextResourceDecoderTest, UTF8Pieces) {
  std::unique_ptr<TextResourceDecoder> decoder =
      TextResourceDecoder::Create(TextResourceDecoderOptions(
          TextResourceDecoderOptions::kPlainTextContent, UTF8Encoding()));

  WTF::String decoded;
  const unsigned char kFoo[] = {
      0x66,                    // f
      0x6f,                    // o
      0xf0, 0x9f, 0x8f, 0x83,  // emoji U+1F3C3 RUNNER
      0x6f                     // o
  };
  const char16_t kFooInUTF16[] = {0x66, 0x6f, 0xd83c, 0xdfc3, 0x6f, 0x00};
  for (char c : kFoo) {
    String chunk = decoder->Decode(reinterpret_cast<const char*>(&c), 1);
    printf("%d [%s]", chunk.length(), chunk.Utf8().data());
    if (chunk.IsNull())
      printf("Null\n");
    else if (chunk.Is8Bit())
      printf("8Bit\n");
    else
      printf("16Bit\n");
    decoded = decoded + chunk;
  }
  decoded = decoded + decoder->Flush();
  EXPECT_EQ(String(kFooInUTF16), decoded);

  std::unique_ptr<TextResourceDecoder> decoder2 =
      TextResourceDecoder::Create(TextResourceDecoderOptions(
          TextResourceDecoderOptions::kPlainTextContent, UTF8Encoding()));

  WTF::String decoded2 =
      decoder->Decode(reinterpret_cast<const char*>(kFoo), 7);
  EXPECT_EQ(String(kFooInUTF16), decoded2);
}

}  // namespace blink
