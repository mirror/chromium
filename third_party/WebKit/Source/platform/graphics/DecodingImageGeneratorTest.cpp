// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/DecodingImageGenerator.h"

#include "platform/image-decoders/ImageDecoderTestHelpers.h"
#include "platform/image-decoders/SegmentReader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

constexpr unsigned kDefaultTestSize = 4 * SharedBuffer::kSegmentSize;
constexpr unsigned kTooShortForSignature = 5;

void PrepareReferenceData(char* buffer, size_t size) {
  for (size_t i = 0; i < size; ++i)
    buffer[i] = static_cast<char>(i);
}

PassRefPtr<SegmentReader> CreateSegmentReader(size_t data_length) {
  char reference_data[data_length];
  PrepareReferenceData(reference_data, sizeof(reference_data));
  RefPtr<SharedBuffer> data = SharedBuffer::Create();
  data->Append(reference_data, sizeof(reference_data));
  return SegmentReader::CreateFromSharedBuffer(std::move(data));
}

}  // namespace

class DecodingImageGeneratorTest : public ::testing::Test {};

TEST_F(DecodingImageGeneratorTest, Create) {
  RefPtr<SharedBuffer> reference_data =
      ReadFile(kDecodersTestingDir, "radient.gif");
  RefPtr<SegmentReader> reader =
      SegmentReader::CreateFromSharedBuffer(std::move(reference_data));
  std::unique_ptr<SkImageGenerator> generator =
      DecodingImageGenerator::Create(reader->GetAsSkData().get());
  // Sanity-check the image to make sure it was loaded.
  EXPECT_EQ(generator->getInfo().width(), 32);
  EXPECT_EQ(generator->getInfo().height(), 32);
}

TEST_F(DecodingImageGeneratorTest, CreateWithNoSize) {
  // Construct dummy image data that produces no valid size from the
  // ImageDecoder.
  EXPECT_EQ(nullptr,
            DecodingImageGenerator::Create(
                CreateSegmentReader(kDefaultTestSize)->GetAsSkData().get()));
}

TEST_F(DecodingImageGeneratorTest, CreateWithNullImageDecoder) {
  // Construct dummy image data that will produce a null image decoder
  // due to data being too short for a signature.
  EXPECT_EQ(
      nullptr,
      DecodingImageGenerator::Create(
          CreateSegmentReader(kTooShortForSignature)->GetAsSkData().get()));
}

// TODO(wkorman): Test Create with a null ImageFrameGenerator. We'd
// need a way to intercept construction of the instance (and could do
// same for ImageDecoder above to reduce fragility of knowing a short
// signature will produce a null ImageDecoder). Note that it's not
// clear that it's possible to end up with a null ImageFrameGenerator,
// so maybe we can just remove that check from
// DecodingImageGenerator::Create.

}  // namespace blink
