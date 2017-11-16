// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/socket_tag.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <winsock2.h>
#include <ws2bth.h>
#elif defined(OS_POSIX)
#include <netinet/in.h>
#endif

#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "base/files/file_util.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/sockaddr_storage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

uint64_t getTaggedBytes(int32_t expected_tag) {
  uint64_t bytes = 0;
  std::string contents;
  EXPECT_TRUE(base::ReadFileToString(
      base::FilePath::FromUTF8Unsafe("/proc/net/xt_qtaguid/stats"), &contents));
  for (size_t i = contents.find('\n');  // Skip first line which is headers.
       i != std::string::npos && i < contents.length();) {
    uint64_t tag, rx_bytes;
    uid_t uid;
    int n;
    ASSERT_EQ(sscanf(contents.c_str() + i,
                     "%*d %*s 0x%" SCNx64 " %d %*d %" SCNu64
                     " %*d %*d %*d %*d %*d %*d %*d %*d "
                     "%*d %*d %*d %*d %*d %*d %*d%n",
                     &tag, &uid, &rx_bytes, &n),
              3);
    if (uid == getuid() && (int32_t)(tag >> 32) == expected_tag) {
      bytes += rx_bytes;
    }
    i += n + 1;
  }
  return bytes;
}

}  // namespace

TEST(SocketTagTest, Compares) {
  SocketTag unset1, unset2;

  EXPECT_TRUE(unset1 == unset2);
  EXPECT_FALSE(unset1 != unset2);
  EXPECT_FALSE(unset1 < unset2);

#if defined(OS_ANDROID)
  SocketTag s00(0, 0), s01(0, 1), s11(1, 1);

  EXPECT_FALSE(s00 == unset1);
  EXPECT_TRUE(s01 != unset2);
  EXPECT_TRUE(unset1 < s00);
  EXPECT_FALSE(s00 < unset2);

  EXPECT_FALSE(s00 == s01);
  EXPECT_FALSE(s01 == s11);
  EXPECT_TRUE(s00 < s01);
  EXPECT_TRUE(s01 < s11);
  EXPECT_TRUE(s00 < s11);
  EXPECT_FALSE(s01 < s00);
  EXPECT_FALSE(s11 < s01);
  EXPECT_FALSE(s11 < s00);
#endif
}

TEST(SocketTagTest, Apply) {
  // Calculate sockaddr for localhost.
  SockaddrStorage addr;
  IPEndPoint end_point(IPAddress::IPv4Localhost(), 80);
  ASSERT_EQ(end_point.ToSockAddr(addr.addr, &addr.addr_len), OK);

  // Create socket.
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  ASSERT_NE(s, -1);

  // Send a UDP packet via a tagged socket verifying traffic is properly
  // accounted for.
  int32_t tag_val1 = 0x12345678;
  uint64_t old_traffic = getTaggedBytes(tag_val1);
  SocketTag tag1(SocketTag::UNSET_UID, tag_val1);
  tag1.Apply(s);
  EXPECT_EQ(sendto(s, &tag_val1, sizeof(tag_val1), 0, addr.addr, addr.addr_len),
            (int)sizeof(tag_val1));
  EXPECT_GT(getTaggedBytes(tag_val1), old_traffic);

  // Repeat test tagging for this UID.
  int32_t tag_val2 = 0x87654321;
  old_traffic = getTaggedBytes(tag_val2);
  SocketTag tag2(getuid(), tag_val2);
  tag2.Apply(s);
  EXPECT_EQ(sendto(s, &tag_val2, sizeof(tag_val2), 0, addr.addr, addr.addr_len),
            (int)sizeof(tag_val2));
  EXPECT_GT(getTaggedBytes(tag_val2), old_traffic);
}

}  // namespace net