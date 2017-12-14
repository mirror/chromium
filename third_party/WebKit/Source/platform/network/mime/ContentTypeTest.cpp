// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/network/mime/ContentType.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(ContentTypeTest, ParameterNameNotFound) {
  ContentType type("text/plain; a");
  EXPECT_TRUE(type.Parameter("b").IsNull());
}

TEST(ContentTypeTest, ParameterValueNotFound) {
  ContentType type("text/plain; a");
  EXPECT_TRUE(type.Parameter("a").IsNull());
}

TEST(ContentTypeTest, Parameter) {
  ContentType type("text/plain; a=foo");
  EXPECT_EQ("foo", type.Parameter("a"));
}

TEST(ContentTypeTest, ParameterWhiteSpaceStripping) {
  ContentType type("text/plain; a= foo ");
  EXPECT_EQ("foo", type.Parameter("a"));
}

TEST(ContentTypeTest, QuotedParameter) {
  ContentType type("text/plain; a=\"foo\"");
  EXPECT_EQ("foo", type.Parameter("a"));
}

TEST(ContentTypeTest, QuotedParameterWhiteSpaceStripping) {
  ContentType type("text/plain; a=\" foo \"");
  EXPECT_EQ("foo", type.Parameter("a"));
}

TEST(ContentTypeTest, ParameterMultiple) {
  ContentType type("text/plain; x = a ; y = b ");
  EXPECT_EQ("a", type.Parameter("x"));
  EXPECT_EQ("b", type.Parameter("y"));
}

// This is for verifying that a bug has been fixed.
TEST(ContentTypeTest, ParameterOnlyOneDoubleQuoteAfterSpace) {
  ContentType type("text/plain; a= \"b");
  EXPECT_TRUE(type.Parameter("a").IsNull());
}

}  // namespace blink
