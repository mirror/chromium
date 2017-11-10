// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include "net/base/host_port_pair.h"
#include "net/base/privacy_mode.h"
#include "net/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// For simplicity, these tests do not create SpdySession instances (necessary
// for a non-null weak pointer) but use nullptr instead.  Streams are identified
// by their SpdyStreamId.

namespace net {
namespace test {
namespace {

class TestDelegate : public Http2PushPromiseIndex::Delegate {
 public:
  TestDelegate() = delete;
  TestDelegate(const SpdySessionKey& key, Http2PushPromiseIndex* index)
      : key_(key), index_(index) {}
  ~TestDelegate() override {}

  bool ValidatePushedStream(const SpdySessionKey& key) const override {
    return key.Equals(key_);
  }

  // SpdySession overrides Delegate::OnPushedStreamClaimed() and deletes claimed
  // streams from its internal index, which calls back to
  // Http2PushPromiseIndex::Unregister().  TestDelegate mimics this behavior.
  void OnPushedStreamClaimed(const GURL& url, SpdyStreamId stream_id) override {
    index_->Unregister(url, nullptr, stream_id);
  }

 private:
  SpdySessionKey key_;
  Http2PushPromiseIndex* index_;
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
              PRIVACY_MODE_ENABLED) {}

  const GURL url1_;
  const GURL url2_;
  const SpdySessionKey key1_;
  const SpdySessionKey key2_;
  Http2PushPromiseIndex index_;
};

}  // namespace

// Trying to unregister a stream not in the index should log to DFATAL.
// Case 1: no streams for the given URL.
TEST_F(Http2PushPromiseIndexTest, UnregisterNonexistingEntryCrashes1) {
  EXPECT_DFATAL(index_.Unregister(url1_, nullptr, 2),
                "Only a previously registered entry can be unregistered.");
}

// Trying to unregister a stream not in the index should log to DFATAL.
// Case 2: there is a stream for the given URL, but not with the same
// (SpdySession, SpdyStreamId) pair.
TEST_F(Http2PushPromiseIndexTest, UnregisterNonexistingEntryCrashes2) {
  TestDelegate delegate(key1_, &index_);
  index_.Register(url1_, nullptr, 2, &delegate);
  EXPECT_DFATAL(index_.Unregister(url1_, nullptr, 4),
                "Only a previously registered entry can be unregistered.");
  // Stream must be unregistered so that Http2PushPromiseIndex destructor
  // does not crash.
  index_.Unregister(url1_, nullptr, 2);
}

// Test that Claim() resets |stream_id| outparam if no stream found.
TEST_F(Http2PushPromiseIndexTest, Empty) {
  base::WeakPtr<SpdySession> spdy_session;
  SpdyStreamId stream_id = 2;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url1_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url2_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url2_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

TEST_F(Http2PushPromiseIndexTest, FindMultipleStreamsWithDifferentUrl) {
  // Test that Claim() works correctly with one stream in the index.
  TestDelegate delegate1(key1_, &index_);
  index_.Register(url1_, nullptr, 2, &delegate1);

  base::WeakPtr<SpdySession> spdy_session;
  SpdyStreamId stream_id = 0;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  // Test that the previous call of Claim() removed the stream from the index.
  stream_id = 2;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  // Test that Claim() works correctly with two streams in the index.
  TestDelegate delegate2(key2_, &index_);
  index_.Register(url1_, nullptr, 2, &delegate1);
  index_.Register(url2_, nullptr, 4, &delegate2);

  // If SpdySessionKey does not match, Claim should not return a stream.
  stream_id = 2;
  index_.Claim(url1_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url2_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  // Claim both streams.
  stream_id = 0;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  stream_id = 0;
  index_.Claim(url2_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  // Test that the previous calls of Claim() removed the streams from the index.
  stream_id = 2;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url2_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

TEST_F(Http2PushPromiseIndexTest,
       MultipleStreamsForSingleUrlWithDifferentKeys) {
  TestDelegate delegate1(key1_, &index_);
  index_.Register(url1_, nullptr, 2, &delegate1);
  TestDelegate delegate2(key2_, &index_);
  index_.Register(url1_, nullptr, 4, &delegate2);

  // Test that no streams are returned for a different URL.
  base::WeakPtr<SpdySession> spdy_session;
  SpdyStreamId stream_id = 2;
  index_.Claim(url2_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url2_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  // Test that the proper stream is returned for |url1_, key2_|.
  stream_id = 0;
  index_.Claim(url1_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  // Test that the previous call of Claim() removed the stream from the index.
  stream_id = 4;
  index_.Claim(url1_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  // Claim the other stream for the same URL.
  stream_id = 4;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  stream_id = 2;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

TEST_F(Http2PushPromiseIndexTest, MultipleStreamsForSingleUrlWithSameKey) {
  TestDelegate delegate1(key1_, &index_);
  index_.Register(url1_, nullptr, 2, &delegate1);
  index_.Register(url1_, nullptr, 4, &delegate1);

  // Test that no streams are returned for a different URL.
  base::WeakPtr<SpdySession> spdy_session;
  SpdyStreamId stream_id = 2;
  index_.Claim(url2_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  stream_id = 2;
  index_.Claim(url2_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  // Test that no streams are returned for a different SpdySessionKey.
  stream_id = 2;
  index_.Claim(url1_, key2_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);

  // First entry in the list wins.
  stream_id = 0;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(2u, stream_id);

  // Claim the other stream for the same URL.
  stream_id = 2;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(4u, stream_id);

  // No more streams.
  stream_id = 4;
  index_.Claim(url1_, key1_, &spdy_session, &stream_id);
  EXPECT_EQ(kNoPushedStreamFound, stream_id);
}

}  // namespace test
}  // namespace net
