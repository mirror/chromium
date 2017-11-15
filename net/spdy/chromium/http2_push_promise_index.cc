// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include <utility>

namespace net {

Http2PushPromiseIndex::Http2PushPromiseIndex() {}

Http2PushPromiseIndex::~Http2PushPromiseIndex() {
  DCHECK(streams_.empty());
}

Http2PushPromiseIndex::PushedStream::PushedStream(
    base::WeakPtr<SpdySession> spdy_session,
    SpdyStreamId stream_id,
    Delegate* delegate)
    : spdy_session(spdy_session), stream_id(stream_id), delegate(delegate) {}

Http2PushPromiseIndex::PushedStream::PushedStream(const PushedStream& other) =
    default;
Http2PushPromiseIndex::PushedStream& Http2PushPromiseIndex::PushedStream::
operator=(const Http2PushPromiseIndex::PushedStream& other) = default;

Http2PushPromiseIndex::PushedStream::~PushedStream() {}

void Http2PushPromiseIndex::Register(const GURL& url,
                                     base::WeakPtr<SpdySession> spdy_session,
                                     SpdyStreamId stream_id,
                                     Delegate* delegate) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  // Use lower_bound() so that if key does not exists, then insertion can use
  // its return value as a hint.
  StreamMap::iterator url_it = streams_.lower_bound(url);
  if (url_it == streams_.end() || url_it->first != url) {
    streams_.insert(
        url_it, std::make_pair(url, StreamList{PushedStream(
                                        spdy_session, stream_id, delegate)}));
    return;
  }
  url_it->second.push_back(PushedStream(spdy_session, stream_id, delegate));
}

void Http2PushPromiseIndex::Unregister(const GURL& url,
                                       SpdySession* spdy_session,
                                       SpdyStreamId stream_id) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  StreamMap::iterator url_it = streams_.find(url);
  if (url_it == streams_.end()) {
    LOG(DFATAL) << "Only a previously registered entry can be unregistered.";
    return;
  }
  for (StreamList::iterator it = url_it->second.begin();
       it != url_it->second.end();) {
    if (it->spdy_session.get() == spdy_session && it->stream_id == stream_id) {
      url_it->second.erase(it);
      if (url_it->second.empty()) {
        streams_.erase(url_it);
      }
      return;
    }
    ++it;
  }
  LOG(DFATAL) << "Only a previously registered entry can be unregistered.";
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

  for (const auto& pushed_stream : url_it->second) {
    if (!pushed_stream.delegate->ValidatePushedStream(key)) {
      continue;
    }
    // Return pushed stream in outparams.
    *spdy_session = pushed_stream.spdy_session;
    *stream_id = pushed_stream.stream_id;
    // Notify Delegate that pushed stream is claimed.
    pushed_stream.delegate->OnPushedStreamClaimed(url, pushed_stream.stream_id);
    // Delegate might call Http2PushPromiseIndex::Unregister(), therefore at
    // this point, |pushed_stream| might be invalid.
    return;
  }
}

}  // namespace net
