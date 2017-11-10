// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_
#define NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/base/net_export.h"
#include "net/spdy/chromium/spdy_session_key.h"
#include "net/spdy/core/spdy_protocol.h"
#include "url/gurl.h"

namespace net {

class SpdySession;

// This value means that Claim() has not found any matching pushed streams.
const SpdyStreamId kNoPushedStreamFound = 0;

// This class manages cross-origin unclaimed pushed streams (push promises) from
// the receipt of PUSH_PROMISE frame until they are matched to a request.  An
// unclaimed pushed stream is identified by a WeakPtr<SpdySession> and a
// SpdyStreamId tuple.  In addition, a Delegate* is stored along with each
// unclaimed pushed stream.
// Each SpdySessionPool owns one instance of this class, which then allows
// requests to be matched with a pushed stream regardless of which HTTP/2
// connection the stream is on.
// Only pushed streams with cryptographic schemes (for example, https) are
// allowed to be shared across connections.  Non-cryptographic scheme pushes
// (for example, http) are fully managed within each SpdySession.
class NET_EXPORT Http2PushPromiseIndex {
 public:
  // Interface for validating pushed streams and signaling when a pushed stream
  // is claimed.
  class NET_EXPORT Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    // Return true if the pushed stream can be used for a request with |key|.
    virtual bool ValidatePushedStream(const SpdySessionKey& key) const = 0;

    // Called when a pushed stream is claimed.
    virtual void OnPushedStreamClaimed(const GURL& url,
                                       SpdyStreamId stream_id) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  Http2PushPromiseIndex();
  ~Http2PushPromiseIndex();

  // Registers an unclaimed pushed stream for |url|.
  void Register(const GURL& url,
                base::WeakPtr<SpdySession> spdy_session,
                SpdyStreamId stream_id,
                Delegate* delegate);

  // Unregisters an unclaimed pushed stream for |url|.  The pushed stream must
  // exist in the index.
  void Unregister(const GURL& url,
                  SpdySession* spdy_session,
                  SpdyStreamId stream_id);

  // Looks for a matching unclaimed pushed stream for |url| that can be used by
  // an HttpStreamFactoryImpl::Job with |key|.  If found, |*spdy_session| and
  // |*stream_id| are set to point to this stream, and the stream is removed
  // from the index.  If not found, |*spdy_session| is set to nullptr and
  // |*stream_id| is set to kNoPushedStreamFound.
  void Claim(const GURL& url,
             const SpdySessionKey& key,
             base::WeakPtr<SpdySession>* spdy_session,
             SpdyStreamId* stream_id);

 private:
  struct PushedStream {
    PushedStream() = delete;
    PushedStream(base::WeakPtr<SpdySession> spdy_session,
                 SpdyStreamId stream_id,
                 Delegate* delegate);
    PushedStream(const PushedStream& other);
    PushedStream& operator=(const PushedStream& other);
    ~PushedStream();

    base::WeakPtr<SpdySession> spdy_session;
    SpdyStreamId stream_id;
    Delegate* delegate;
  };
  using StreamList = std::vector<PushedStream>;
  using StreamMap = std::map<GURL, StreamList>;

  // A multimap of unclaimed pushed streams keyed off GURL.  SpdySession must
  // unregister its streams before destruction, therefore all weak pointers must
  // be valid, except for tests.
  StreamMap streams_;

  DISALLOW_COPY_AND_ASSIGN(Http2PushPromiseIndex);
};

}  // namespace net

#endif  // NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_
