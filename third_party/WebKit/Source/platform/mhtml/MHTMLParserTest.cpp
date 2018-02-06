// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mhtml/MHTMLParser.h"

#include "platform/SharedBuffer.h"
#include "platform/mhtml/ArchiveResource.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
std::string ResourceData(ArchiveResource* resource) {
  Vector<char> data = resource->Data()->Copy();
  return std::string(data.data(), data.size());
}

TEST(MHTMLParserTest, TransferEncodings) {
  const char mhtml_data[] =
      "Date: Wed, 06 Dec 2017 02:14:52 GMT\r\n"
      "From: <Saved by Blink>\r\n"
      "Subject: Test Subject\r\n"
      "MIME-Version: 1.0\r\n"
      "Snapshot-Content-Location: https://chromium.org/\r\n"
      "Content-Type: multipart/related;\r\n"
      "\ttype=\"text/html\";\r\n"
      "\tboundary=\"BoUnDaRy\"\r\n"
      "\r\n"
      "\r\n"
      "--BoUnDaRy\r\n"
      "Content-Location: http://chromium.org/\r\n"
      "Content-Transfer-Encoding: quoted-printable\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "\r\n"
      "text\r\n"
      "=\r\n"
      "--BoUnDaRy\r\n"
      "Content-Location: https://chromium.org/binary\r\n"
      "Content-Transfer-Encoding: binary\r\n"
      "Content-Type: binary/octet-stream\r\n"
      "\r\n"
      "bin\0ary\r\n"
      "--BoUnDaRy\r\n"
      "Content-Location: http://chromium.org/base64\r\n"
      "Content-Transfer-Encoding: base64\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "\r\n"
      "YmFzZTY0\r\n"
      "--BoUnDaRy\r\n"
      "Content-Location: https://chromium.org/emptybinary\r\n"
      "Content-Transfer-Encoding: binary\r\n"
      "Content-Type: binary/octet-stream\r\n"
      "\r\n"
      "--BoUnDaRy--\r\n";

  scoped_refptr<SharedBuffer> buf =
      blink::SharedBuffer::Create(mhtml_data, sizeof(mhtml_data));
  MHTMLParser parser(buf);
  HeapVector<Member<ArchiveResource>> resources = parser.ParseArchive();
  EXPECT_EQ(4ul, resources.size());

  EXPECT_EQ("text\r\n", ResourceData(resources[0].Get()));
  EXPECT_EQ(std::string("bin\0ary", 7), ResourceData(resources[1].Get()));
  EXPECT_EQ("base64", ResourceData(resources[2].Get()));
  EXPECT_EQ("", ResourceData(resources[3].Get()));
}

}  // namespace blink
