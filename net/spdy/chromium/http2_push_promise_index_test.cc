// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include <utility>

#include "base/run_loop.h"
#include "net/base/host_port_pair.h"
#include "net/base/privacy_mode.h"
#include "net/log/test_net_log.h"
#include "net/socket/socket_test_util.h"
#include "net/spdy/chromium/spdy_session.h"
#include "net/spdy/chromium/spdy_test_util_common.h"
#include "net/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {
namespace test {
namespace {

// Entry implementation with Validate() requiring exact match of SpdySessionKey.
class TestEntry : public Http2PushPromiseIndex::Entry {
 public:
  TestEntry() = delete;
  TestEntry(base::WeakPtr<SpdySession> spdy_session, const SpdySessionKey& key)
      : spdy_session_(spdy_session), key_(key) {}
  ~TestEntry() override {}

  bool Validate(const SpdySessionKey& key) const override {
    return key.Equals(key_);
  }
  base::WeakPtr<SpdySession> spdy_session() const override {
    return spdy_session_;
  }

 private:
  base::WeakPtr<SpdySession> spdy_session_;
  const SpdySessionKey key_;
};

class Http2PushPromiseIndexTest : public testing::Test {
 protected:
  Http2PushPromiseIndexTest()
      : url1_("https://www.example.org"),
        url2_("https://mail.example.com"),
        key1_(HostPortPair::FromURL(url1_),
              ProxyServer::Direct(),
              PRIVACY_MODE_ENABLED),
        key2_(HostPortPair::FromURL(url2_),
              ProxyServer::Direct(),
              PRIVACY_MODE_ENABLED),
        http_network_session_(
            SpdySessionDependencies::SpdyCreateSession(&session_deps_)) {}

  NetLogWithSource log_;
  const GURL url1_;
  const GURL url2_;
  const SpdySessionKey key1_;
  const SpdySessionKey key2_;
  SpdySessionDependencies session_deps_;
  std::unique_ptr<HttpNetworkSession> http_network_session_;
  Http2PushPromiseIndex index_;
};

}  // namespace

TEST_F(Http2PushPromiseIndexTest, Empty) {
  EXPECT_FALSE(index_.Find(key1_, url1_));
  EXPECT_FALSE(index_.Find(key1_, url2_));
  EXPECT_FALSE(index_.Find(key2_, url1_));
  EXPECT_FALSE(index_.Find(key2_, url2_));
}

TEST_F(Http2PushPromiseIndexTest, UnregisterNonexistingEntryCrashes) {
  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  SequencedSocketData data(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  base::WeakPtr<SpdySession> spdy_session =
      CreateSpdySession(http_network_session_.get(), key1_, log_);
  // Read hanging socket data.
  base::RunLoop().RunUntilIdle();

  // Create entry but do not register it.
  TestEntry entry(spdy_session, key1_);

  // Try to unregister entry not in the index.
  EXPECT_DFATAL(index_.Unregister(url1_, entry),
                "Check failed: false. Only a previously registered entry can "
                "be unregistered.");

  EXPECT_TRUE(data.AllReadDataConsumed());
  EXPECT_TRUE(data.AllWriteDataConsumed());
}

TEST_F(Http2PushPromiseIndexTest, FindMultipleSessionsWithDifferentUrl) {
  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  // For first session.
  SequencedSocketData data1(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data1);
  // For second session.
  SequencedSocketData data2(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data2);

  base::WeakPtr<SpdySession> spdy_session1 =
      CreateSpdySession(http_network_session_.get(), key1_, log_);
  base::WeakPtr<SpdySession> spdy_session2 =
      CreateSpdySession(http_network_session_.get(), key2_, log_);
  // Read hanging socket data.
  base::RunLoop().RunUntilIdle();

  auto entry1 = std::make_unique<TestEntry>(spdy_session1, key1_);
  TestEntry* entry1_raw = entry1.get();
  auto entry2 = std::make_unique<TestEntry>(spdy_session2, key2_);
  TestEntry* entry2_raw = entry2.get();

  index_.Register(url1_, std::move(entry1));
  index_.Register(url2_, std::move(entry2));

  EXPECT_EQ(spdy_session1.get(), index_.Find(key1_, url1_).get());
  EXPECT_FALSE(index_.Find(key1_, url2_));
  EXPECT_FALSE(index_.Find(key2_, url1_));
  EXPECT_EQ(spdy_session2.get(), index_.Find(key2_, url2_).get());

  index_.Unregister(url1_, *entry1_raw);
  index_.Unregister(url2_, *entry2_raw);

  EXPECT_FALSE(index_.Find(key1_, url1_));
  EXPECT_FALSE(index_.Find(key1_, url2_));
  EXPECT_FALSE(index_.Find(key2_, url1_));
  EXPECT_FALSE(index_.Find(key2_, url2_));

  // SpdySession weak pointers must still be valid,
  // otherwise comparisons above are not meaningful.
  EXPECT_TRUE(spdy_session1);
  EXPECT_TRUE(spdy_session2);

  EXPECT_TRUE(data1.AllReadDataConsumed());
  EXPECT_TRUE(data1.AllWriteDataConsumed());
  EXPECT_TRUE(data2.AllReadDataConsumed());
  EXPECT_TRUE(data2.AllWriteDataConsumed());
}

TEST_F(Http2PushPromiseIndexTest, MultipleSessionsForSameUrlWithDifferentKeys) {
  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  // For first session.
  SequencedSocketData data1(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data1);
  // For second session.
  SequencedSocketData data2(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data2);

  base::WeakPtr<SpdySession> spdy_session1 =
      CreateSpdySession(http_network_session_.get(), key1_, log_);
  base::WeakPtr<SpdySession> spdy_session2 =
      CreateSpdySession(http_network_session_.get(), key2_, log_);
  // Read hanging socket data.
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(index_.Find(key1_, url1_));
  EXPECT_FALSE(index_.Find(key2_, url1_));

  auto entry1 = std::make_unique<TestEntry>(spdy_session1, key1_);
  TestEntry* entry1_raw = entry1.get();
  auto entry2 = std::make_unique<TestEntry>(spdy_session2, key2_);
  TestEntry* entry2_raw = entry2.get();

  index_.Register(url1_, std::move(entry1));
  index_.Register(url1_, std::move(entry2));

  EXPECT_EQ(spdy_session1.get(), index_.Find(key1_, url1_).get());
  EXPECT_EQ(spdy_session2.get(), index_.Find(key2_, url1_).get());

  index_.Unregister(url1_, *entry1_raw);
  index_.Unregister(url1_, *entry2_raw);

  EXPECT_FALSE(index_.Find(key1_, url1_));
  EXPECT_FALSE(index_.Find(key2_, url1_));

  // SpdySession weak pointers must still be valid,
  // otherwise comparisons above are not meaningful.
  EXPECT_TRUE(spdy_session1);
  EXPECT_TRUE(spdy_session2);

  EXPECT_TRUE(data1.AllReadDataConsumed());
  EXPECT_TRUE(data1.AllWriteDataConsumed());
  EXPECT_TRUE(data2.AllReadDataConsumed());
  EXPECT_TRUE(data2.AllWriteDataConsumed());
}

TEST_F(Http2PushPromiseIndexTest, MultipleSessionsForSameUrlWithSameKey) {
  MockRead reads[] = {MockRead(SYNCHRONOUS, ERR_IO_PENDING, 0)};
  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  // For first session.
  SequencedSocketData data1(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data1);
  // For second session.
  SequencedSocketData data2(reads, arraysize(reads), nullptr, 0);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.socket_factory->AddSocketDataProvider(&data2);

  base::WeakPtr<SpdySession> spdy_session1 =
      CreateSpdySession(http_network_session_.get(), key1_, log_);
  base::WeakPtr<SpdySession> spdy_session2 =
      CreateSpdySession(http_network_session_.get(), key2_, log_);
  // Read hanging socket data.
  base::RunLoop().RunUntilIdle();

  // The two entries will match the same SpdySessionKey.
  auto entry1 = std::make_unique<TestEntry>(spdy_session1, key1_);
  TestEntry* entry1_raw = entry1.get();
  auto entry2 = std::make_unique<TestEntry>(spdy_session2, key1_);
  TestEntry* entry2_raw = entry2.get();

  EXPECT_FALSE(index_.Find(key1_, url1_));

  index_.Register(url1_, std::move(entry1));

  EXPECT_EQ(spdy_session1.get(), index_.Find(key1_, url1_).get());

  index_.Register(url1_, std::move(entry2));

  // First entry in list wins.
  EXPECT_EQ(spdy_session1.get(), index_.Find(key1_, url1_).get());

  index_.Unregister(url1_, *entry1_raw);

  // After first entry is removed, second entry is returned.
  EXPECT_EQ(spdy_session2.get(), index_.Find(key1_, url1_).get());

  index_.Unregister(url1_, *entry2_raw);

  EXPECT_FALSE(index_.Find(key1_, url1_));

  // SpdySession weak pointers must still be valid,
  // otherwise comparisons above are not meaningful.
  EXPECT_TRUE(spdy_session1);
  EXPECT_TRUE(spdy_session2);

  EXPECT_TRUE(data1.AllReadDataConsumed());
  EXPECT_TRUE(data1.AllWriteDataConsumed());
  EXPECT_TRUE(data2.AllReadDataConsumed());
  EXPECT_TRUE(data2.AllWriteDataConsumed());
}

}  // namespace test
}  // namespace net
