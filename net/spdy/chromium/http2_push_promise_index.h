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

// This class manages cross-origin pushed streams from the receipt of
// PUSH_PROMISE frame until they are matched to a request.  Each SpdySessionPool
// owns one instance of this class, which then allows requests to be matched
// with a pushed stream regardless of which HTTP/2 connection the stream is on
// on.  Only pushed streams with cryptographic schemes (for example, https) are
// allowed to be shared across connections.  Non-cryptographic scheme pushes
// (for example, http) are fully managed within each SpdySession.
class NET_EXPORT Http2PushPromiseIndex {
 public:
  Http2PushPromiseIndex();
  ~Http2PushPromiseIndex();

  // Registers a SpdySession with an unclaimed pushed stream for |url|.
  // Called by SpdySession when pushed stream is opened by server.
  void Register(const GURL& url,
                base::WeakPtr<SpdySession> spdy_session,
                SpdyStreamId stream_id);

  // Unregisters a SpdySession with an unclaimed pushed stream for |url|.
  // Such pushed stream must exist in the index.
  // Called by SpdySession when pushed stream is closed by server, or when push
  // is cancelled because resource is in the cache.
  void Unregister(const GURL& url,
                  base::WeakPtr<SpdySession> spdy_session,
                  SpdyStreamId stream_id);

  // Looks for a matching unclaimed pushed stream for |url| that can be used by
  // an HttpStreamFactoryImpl::Job with |key|.
  // If found, |*spdy_session| and |*stream_id| are set to point to this stream,
  // which is removed from the index.
  // If not found, |*spdy_session| is set to nullptr and |*stream_id| is set to
  // kNoPushedStreamFound.
  // Called by HttpStreamFactoryImpl::Job.
  void Claim(const GURL& url,
             const SpdySessionKey& key,
             base::WeakPtr<SpdySession>* spdy_session,
             SpdyStreamId* stream_id);

 private:
  struct PushedStream {
    PushedStream() = delete;
    PushedStream(base::WeakPtr<SpdySession> spdy_session,
                 SpdyStreamId stream_id);
    PushedStream(const PushedStream& other);
    PushedStream& operator=(const PushedStream& other);
    ~PushedStream();

    base::WeakPtr<SpdySession> spdy_session;
    SpdyStreamId stream_id;
  };
  using StreamList = std::vector<PushedStream>;
  using StreamMap = std::map<GURL, StreamList>;

  // A map of all SpdySessions owned by |this| that have an unclaimed pushed
  // streams for a GURL.  Might contain invalid WeakPtr's.
  // A single SpdySession can only have at most one pushed stream for each GURL,
  // but it is possible that multiple SpdySessions have pushed streams for the
  // same GURL.
  StreamMap streams_;

  DISALLOW_COPY_AND_ASSIGN(Http2PushPromiseIndex);
};

}  // namespace net

#endif  // NET_SPDY_CHROMIUM_HTTP2_PUSH_PROMISE_INDEX_H_
