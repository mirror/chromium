// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/testing/MockWebCrypto.h"

#include <cstring>
#include <memory>
#include <string>
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

MATCHER_P2(MemEq,
           p,
           len,
           std::string("pointing to memory") + (negation ? " not" : "") +
               " equal to \"" + std::string(static_cast<const char*>(p), len) +
               "\" (length=" + ::testing::PrintToString(len) + ")") {
  return memcmp(arg, p, len) == 0;
}

void MockWebCryptoDigestor::ExpectConsumeAndFinish(const void* input_data,
                                                   unsigned input_length,
                                                   void* output_data,
                                                   unsigned output_length) {
  ::testing::InSequence s;
  EXPECT_CALL(*this, Consume(MemEq(input_data, input_length), input_length))
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*this, Finish(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(
          ::testing::SetArgReferee<0>(static_cast<unsigned char*>(output_data)),
          ::testing::SetArgReferee<1>(output_length), ::testing::Return(true)));
}

MockWebCryptoDigestorFactory::MockWebCryptoDigestorFactory(
    const void* input_data,
    unsigned input_length,
    void* output_data,
    unsigned output_length)
    : input_data_(input_data),
      input_length_(input_length),
      output_data_(output_data),
      output_length_(output_length) {}

MockWebCryptoDigestor* MockWebCryptoDigestorFactory::Create() {
  std::unique_ptr<MockWebCryptoDigestor> digestor(
      MockWebCryptoDigestor::Create());
  digestor->ExpectConsumeAndFinish(input_data_, input_length_, output_data_,
                                   output_length_);
  return digestor.release();
}

}  // namespace blink
