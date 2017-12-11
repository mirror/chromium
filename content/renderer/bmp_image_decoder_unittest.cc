// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "content/test/image_decoder_test.h"
#include "third_party/WebKit/public/web/WebImageDecoder.h"

class BMPImageDecoderTest : public ImageDecoderTest {
 public:
  BMPImageDecoderTest() : ImageDecoderTest("bmp") { }

 protected:
  blink::WebImageDecoder* CreateWebKitImageDecoder() const override {
    return new blink::WebImageDecoder(blink::WebImageDecoder::kTypeBMP);
  }

  // The BMPImageDecoderTest tests are really slow under TSAN etc.
  // Thus it is split into fast and slow versions. The threshold is
  // set to 10KB because the fast test should be able to finish in
  // less than 30 seconds, even with TSAN.
  static const int64_t kThresholdSize = 10240;
};

TEST_F(BMPImageDecoderTest, DecodingFast) {
  TestDecoding(TEST_SMALLER, kThresholdSize);
}

#if defined(THREAD_SANITIZER)
// BMPImageDecoderTest.DecodingSlow always times out under ThreadSanitizer v2.
#define MAYBE_DecodingSlow DISABLED_DecodingSlow
#else
#define MAYBE_DecodingSlow DecodingSlow
#endif
TEST_F(BMPImageDecoderTest, MAYBE_DecodingSlow) {
  TestDecoding(TEST_BIGGER, kThresholdSize);
}
