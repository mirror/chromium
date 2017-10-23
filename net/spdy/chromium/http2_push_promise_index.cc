// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include <utility>

#include "net/spdy/chromium/spdy_session.h"

namespace net {

Http2PushPromiseIndex::Http2PushPromiseIndex() {}
Http2PushPromiseIndex::~Http2PushPromiseIndex() {}

Http2PushPromiseIndex::PushedStream::PushedStream(
    base::WeakPtr<SpdySession> spdy_session,
    SpdyStreamId stream_id)
    : spdy_session(spdy_session), stream_id(stream_id) {}

Http2PushPromiseIndex::PushedStream::PushedStream(const PushedStream& other) =
    default;
Http2PushPromiseIndex::PushedStream& Http2PushPromiseIndex::PushedStream::
operator=(const Http2PushPromiseIndex::PushedStream& other) = default;

Http2PushPromiseIndex::PushedStream::~PushedStream() {}

void Http2PushPromiseIndex::Register(const GURL& url,
                                     base::WeakPtr<SpdySession> spdy_session,
                                     SpdyStreamId stream_id) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  // Use lower_bound() so that if key does not exists, then insertion can use
  // its return value as a hint.
  StreamMap::iterator url_it = streams_.lower_bound(url);
  if (url_it == streams_.end() || url_it->first != url) {
    streams_.insert(
        url_it,
        std::make_pair(url, StreamList{PushedStream(spdy_session, stream_id)}));
    return;
  }
  url_it->second.push_back(PushedStream(spdy_session, stream_id));
}

void Http2PushPromiseIndex::Unregister(const GURL& url,
                                       base::WeakPtr<SpdySession> spdy_session,
                                       SpdyStreamId stream_id) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  StreamMap::iterator url_it = streams_.find(url);
  DCHECK(url_it != streams_.end());
  size_t removed = 0;
  for (StreamList::iterator it = url_it->second.begin();
       it != url_it->second.end();) {
    // Lazy deletion of destroyed SpdySessions.
    if (!it->spdy_session) {
      it = url_it->second.erase(it);
      continue;
    }
    if (it->spdy_session.get() == spdy_session.get()) {
      DCHECK_EQ(stream_id, it->stream_id);
      it = url_it->second.erase(it);
      ++removed;
      break;
    }
    ++it;
  }
  if (url_it->second.empty()) {
    streams_.erase(url_it);
  }
  DCHECK_EQ(1u, removed);
}

void Http2PushPromiseIndex::Claim(const GURL& url,
                                  const SpdySessionKey& key,
                                  base::WeakPtr<SpdySession>* spdy_session,
                                  SpdyStreamId* stream_id) {
  DCHECK(!url.is_empty());

  *spdy_session = nullptr;
  *stream_id = kNoPushedStreamFound;

  StreamMap::iterator url_it = streams_.find(url);
  if (url_it == streams_.end()) {
    return;
  }

  DCHECK(url.SchemeIsCryptographic());

  for (StreamList::iterator it = url_it->second.begin();
       it != url_it->second.end();) {
    // Lazy deletion of destroyed SpdySessions.
    if (!it->spdy_session) {
      it = url_it->second.erase(it);
      continue;
    }
    const SpdySessionKey& spdy_session_key =
        it->spdy_session->spdy_session_key();
    if (spdy_session_key.proxy_server() != key.proxy_server() ||
        spdy_session_key.privacy_mode() != key.privacy_mode()) {
      ++it;
      continue;
    }
    if (!it->spdy_session->VerifyDomainAuthentication(
            key.host_port_pair().host())) {
      ++it;
      continue;
    }
    // Return pushed stream in outparams.
    *spdy_session = it->spdy_session;
    *stream_id = it->stream_id;
    // Notify SpdySession that pushed stream is claimed.
    (*spdy_session)->ClaimPushStream(url);
    // SpdySession::ClaimPushStream() calls Http2PushPromiseIndex::Unregister(),
    // so at this point, |it| is invalid.
    return;
  }
}

}  // namespace net
