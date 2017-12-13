// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_
#define NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_

#include <set>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/base/net_export.h"
#include "net/spdy/chromium/spdy_session_key.h"
#include "net/spdy/core/spdy_protocol.h"
#include "url/gurl.h"

namespace net {

class SpdySession;

namespace test {

class Http2PushPromiseIndexPeer;

}  // namespace test

// Value returned by FindSession() and FindStream() if no stream is found.
const SpdyStreamId kNoPushedStreamFound = 0;

// This class manages unclaimed pushed streams (push promises) from the receipt
// of PUSH_PROMISE frame until they are matched to a request.  Each
// SpdySessionPool owns one instance of this class.  SpdySession uses this class
// to register, unregister, and query pushed streams.
// HttpStreamFactoryImpl::Job uses this class to find a SpdySession with a
// pushed stream matching the request, if such exists, which is only allowed for
// requests with a cryptographic scheme.
class NET_EXPORT Http2PushPromiseIndex {
 public:
  // Interface for validating pushed streams, signaling when a pushed stream is
  // claimed, and generating SpdySession weak pointer.
  class NET_EXPORT Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    // Return true if the pushed stream can be used for a request with |key|.
    virtual bool ValidatePushedStream(const SpdySessionKey& key) const = 0;

    // Called when a pushed stream is claimed.  Guaranateed to be called
    // synchronously after ValidatePushedStream() is called and returns true.
    virtual void OnPushedStreamClaimed(const GURL& url,
                                       SpdyStreamId stream_id) = 0;

    // Generate weak pointer.  Guaranateed to be called synchronously after
    // ValidatePushedStream() is called and returns true.
    virtual base::WeakPtr<SpdySession> GetWeakPtrToSession() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  Http2PushPromiseIndex();
  ~Http2PushPromiseIndex();

  // Tries to register a Delegate with an unclaimed pushed stream for |url|.
  // Caller must make sure |delegate| stays valid by unregistering the exact
  // same entry before |delegate| is destroyed.
  // Returns true if there is no unclaimed pushed stream with the same URL for
  // the same Delegate, in which case the stream is registered.
  bool RegisterUnclaimedPushedStream(const GURL& url,
                                     SpdyStreamId stream_id,
                                     Delegate* delegate) WARN_UNUSED_RESULT;

  // Tries to unregister a Delegate with an unclaimed pushed stream for |url|
  // with given |stream_id|.
  // Returns true if this exact entry is found, in which case it is removed.
  bool UnregisterUnclaimedPushedStream(const GURL& url,
                                       SpdyStreamId stream_id,
                                       Delegate* delegate) WARN_UNUSED_RESULT;

  // Returns the number of pushed streams registered for |delegate|.
  size_t CountStreamsForSession(const Delegate* delegate) const;

  // Returns the stream ID of the entry registered for |delegate| with |url|,
  // or kNoPushedStreamFound if no such entry exists.
  SpdyStreamId FindStream(const GURL& url, const Delegate* delegate) const;

  // If there exists a session compatible with |key| that has an unclaimed push
  // stream for |url|, then sets |*session| and |*stream| to one such session
  // and stream.  Makes no guarantee on which (session, stream_id) pair it
  // returns if there are multiple matches.  Sets |*session| to nullptr and
  // |*stream| to kNoPushedStreamFound if no such session exists.
  void FindSession(const SpdySessionKey& key,
                   const GURL& url,
                   base::WeakPtr<SpdySession>* session,
                   SpdyStreamId* stream_id) const;

  // Return the estimate of dynamically allocated memory in bytes.
  size_t EstimateMemoryUsage() const;

 private:
  friend test::Http2PushPromiseIndexPeer;

  // An unclaimed pushed stream entry.
  struct NET_EXPORT UnclaimedPushedStream {
    GURL url;
    SpdyStreamId stream_id;
    Delegate* delegate;
    size_t EstimateMemoryUsage() const;
  };

  // Function object satisfying the requirements of "Compare", see
  // http://en.cppreference.com/w/cpp/concept/Compare.
  // A set ordered by this function object supports efficient lookup (O(log n)
  // amortized, O(n) worst case) of the first entry with a given URL, by calling
  // lower_bound() with an entry with that URL and with stream_id =
  // kNoPushedStreamFound.
  struct NET_EXPORT CompareByUrl {
    bool operator()(const UnclaimedPushedStream& a,
                    const UnclaimedPushedStream& b) const;
  };

  // Function object satisfying the requirements of "Compare", see
  // http://en.cppreference.com/w/cpp/concept/Compare.
  // A set ordered by this function object supports efficient lookup (O(log n)
  // amortized, O(n) worst case) of the first entry with a given Delegate, by
  // calling lower_bound() with an entry with that Delegate and an empty GURL.
  // It also supports efficient lookup (O(log n) amortized, O(n) worst case) of
  // the first entry with a given Delegate and URL, by calling lower_bound()
  // with an entry with that Delegate, that URL, and stream_id =
  // kNoPushedStreamFound.
  struct NET_EXPORT CompareByDelegate {
    bool operator()(const UnclaimedPushedStream& a,
                    const UnclaimedPushedStream& b) const;
  };

  // Two sets with identical contents, each holding all unclaimed pushed
  // streams, stored in different order.  Delegate must unregister its streams
  // before destruction, so that all pointers remain valid.  It is possible that
  // multiple Delegates have pushed streams for the same GURL, but for each
  // Delegate there can be at most one entry per URL.
  std::set<UnclaimedPushedStream, CompareByUrl>
      unclaimed_pushed_streams_sorted_by_url_;
  std::set<UnclaimedPushedStream, CompareByDelegate>
      unclaimed_pushed_streams_sorted_by_delegate_;

  DISALLOW_COPY_AND_ASSIGN(Http2PushPromiseIndex);
};

}  // namespace net

#endif  // NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_
