// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include "net/base/host_port_pair.h"
#include "net/base/privacy_mode.h"
#include "net/test/gtest_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// For simplicity, these tests do not create SpdySession instances
// (necessary for a non-null WeakPtr<SpdySession>), instead they use nullptr.
// Streams are identified by SpdyStreamId only.

using ::testing::Return;
using ::testing::_;

namespace net {
namespace test {
namespace {

// Delegate implementation for tests that requires exact match of SpdySessionKey
// in ValidatePushedStream().  Note that SpdySession, unlike TestDelegate,
// allows cross-origin pooling.
class TestDelegate : public Http2PushPromiseIndex::Delegate {
 public:
  TestDelegate() = delete;
  TestDelegate(const SpdySessionKey& key) : key_(key) {}
  ~TestDelegate() override {}

  bool ValidatePushedStream(const SpdySessionKey& key) const override {
    return key == key_;
  }

  void OnPushedStreamClaimed(const GURL& url, SpdyStreamId stream_id) override {
  }

  base::WeakPtr<SpdySession> GetWeakPtrToSession() override { return nullptr; }

 private:
  SpdySessionKey key_;
};

// Mock implementation.
class MockDelegate : public Http2PushPromiseIndex::Delegate {
 public:
  MockDelegate() = default;
  ~MockDelegate() override {}

  MOCK_CONST_METHOD1(ValidatePushedStream, bool(const SpdySessionKey& key));
  MOCK_METHOD2(OnPushedStreamClaimed,
               void(const GURL& url, SpdyStreamId stream_id));

  base::WeakPtr<SpdySession> GetWeakPtrToSession() override { return nullptr; }
};

}  // namespace

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
              PRIVACY_MODE_ENABLED) {}

  const GURL url1_;
  const GURL url2_;
  const SpdySessionKey key1_;
  const SpdySessionKey key2_;
  Http2PushPromiseIndex index_;
};

TEST_F(Http2PushPromiseIndexTest, Empty) {
  base::WeakPtr<SpdySession> session;
  SpdyStreamId stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.FindSession(key1_, url2_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.FindSession(key1_, url2_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.FindSession(key2_, url2_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

// Trying to unregister a stream not in the index should log to DFATAL.
// Case 1: no streams for the given URL.
TEST_F(Http2PushPromiseIndexTest, UnregisterNonexistingEntryCrashes1) {
  TestDelegate delegate(key1_);
  EXPECT_DFATAL(index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate),
                "Only a previously registered entry can be unregistered.");
}

// Trying to unregister a stream not in the index should log to DFATAL.
// Case 2: there is a stream for the given URL, but not with the same stream ID.
TEST_F(Http2PushPromiseIndexTest, UnregisterNonexistingEntryCrashes2) {
  TestDelegate delegate(key1_);
  index_.RegisterUnclaimedPushedStream(url1_, 2, &delegate);
  EXPECT_DFATAL(index_.UnregisterUnclaimedPushedStream(url1_, 4, &delegate),
                "Only a previously registered entry can be unregistered.");
  // Stream must be unregistered so that Http2PushPromiseIndex destructor
  // does not crash.
  index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate);
}

TEST_F(Http2PushPromiseIndexTest, FindMultipleStreamsWithDifferentUrl) {
  TestDelegate delegate(key1_);
  index_.RegisterUnclaimedPushedStream(url1_, 2, &delegate);

  base::WeakPtr<SpdySession> session;
  SpdyStreamId stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  stream_id = 2;
  index_.FindSession(key1_, url2_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  index_.RegisterUnclaimedPushedStream(url2_, 4, &delegate);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url2_, &session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate);

  stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url2_, &session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  index_.UnregisterUnclaimedPushedStream(url2_, 4, &delegate);

  stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.FindSession(key1_, url2_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

TEST_F(Http2PushPromiseIndexTest, MultipleStreamsWithDifferentKeys) {
  TestDelegate delegate1(key1_);
  index_.RegisterUnclaimedPushedStream(url1_, 2, &delegate1);

  base::WeakPtr<SpdySession> session;
  SpdyStreamId stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  stream_id = 2;
  index_.FindSession(key2_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  TestDelegate delegate2(key2_);
  index_.RegisterUnclaimedPushedStream(url1_, 4, &delegate2);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key2_, url1_, &session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate1);

  stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key2_, url1_, &session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 4, &delegate2);

  stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.FindSession(key2_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

TEST_F(Http2PushPromiseIndexTest, MultipleMatchingStreams) {
  TestDelegate delegate(key1_);
  index_.RegisterUnclaimedPushedStream(url1_, 2, &delegate);
  index_.RegisterUnclaimedPushedStream(url1_, 4, &delegate);

  base::WeakPtr<SpdySession> session;
  SpdyStreamId stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  // FindSession() makes no guarantee about which stream it returns
  // if there are multiple for the same URL.
  EXPECT_NE(kNoPushedStreamFound, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate);

  stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 4, &delegate);

  stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

TEST_F(Http2PushPromiseIndexTest, MatchCallsOnPushedStreamClaimed) {
  MockDelegate delegate;
  EXPECT_CALL(delegate, ValidatePushedStream(key1_)).WillOnce(Return(true));
  EXPECT_CALL(delegate, OnPushedStreamClaimed(url1_, 2)).Times(1);

  index_.RegisterUnclaimedPushedStream(url1_, 2, &delegate);

  base::WeakPtr<SpdySession> session;
  SpdyStreamId stream_id = kNoPushedStreamFound;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate);
};

TEST_F(Http2PushPromiseIndexTest, MismatchDoesNotCallOnPushedStreamClaimed) {
  MockDelegate delegate;
  EXPECT_CALL(delegate, ValidatePushedStream(key1_)).WillOnce(Return(false));
  EXPECT_CALL(delegate, OnPushedStreamClaimed(_, _)).Times(0);

  index_.RegisterUnclaimedPushedStream(url1_, 2, &delegate);

  base::WeakPtr<SpdySession> session;
  SpdyStreamId stream_id = 2;
  index_.FindSession(key1_, url1_, &session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  index_.UnregisterUnclaimedPushedStream(url1_, 2, &delegate);
};

}  // namespace test
}  // namespace net
