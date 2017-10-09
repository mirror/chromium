// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include <utility>

#include "net/spdy/chromium/spdy_session.h"

namespace net {

Http2PushPromiseIndex::Http2PushPromiseIndex() {}
Http2PushPromiseIndex::~Http2PushPromiseIndex() {}

base::WeakPtr<SpdySession> Http2PushPromiseIndex::Find(
    const SpdySessionKey& key,
    const GURL& url) {
  DCHECK(!url.is_empty());

  if (!url.SchemeIsCryptographic()) {
    return base::WeakPtr<SpdySession>();
  }

  UnclaimedPushedStreamMap::iterator it = unclaimed_pushed_streams_.find(url);
  if (it == unclaimed_pushed_streams_.end()) {
    return base::WeakPtr<SpdySession>();
  }

  base::WeakPtr<SpdySession> spdy_session = it->second;
  // Lazy deletion of destroyed SpdySessions.
  if (!spdy_session) {
    unclaimed_pushed_streams_.erase(it);
    return base::WeakPtr<SpdySession>();
  }

  const SpdySessionKey& spdy_session_key = spdy_session->spdy_session_key();
  if (!(spdy_session_key.proxy_server() == key.proxy_server()) ||
      !(spdy_session_key.privacy_mode() == key.privacy_mode())) {
    return base::WeakPtr<SpdySession>();
  }

  if (!spdy_session->VerifyDomainAuthentication(key.host_port_pair().host())) {
    return base::WeakPtr<SpdySession>();
  }

  return spdy_session;
}

bool Http2PushPromiseIndex::RegisterUnclaimedPushedStream(
    const GURL& url,
    base::WeakPtr<SpdySession> spdy_session) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  auto result = unclaimed_pushed_streams_.insert(
      std::make_pair(url, std::move(spdy_session)));
  return result.second;
}

void Http2PushPromiseIndex::UnregisterUnclaimedPushedStream(const GURL& url) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  size_t number_of_erased_elements = unclaimed_pushed_streams_.erase(url);
  DCHECK_EQ(1u, number_of_erased_elements);
}

}  // namespace net
