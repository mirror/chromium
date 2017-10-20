// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include <utility>

#include "net/spdy/chromium/spdy_session.h"

namespace net {

Http2PushPromiseIndex::Http2PushPromiseIndex() {}
Http2PushPromiseIndex::~Http2PushPromiseIndex() {}

SpdySession* Http2PushPromiseIndex::Find(const SpdySessionKey& key,
                                         const GURL& url) {
  DCHECK(!url.is_empty());

  UnclaimedPushedStreamMap::iterator url_it =
      unclaimed_pushed_streams_.find(url);

  if (url_it == unclaimed_pushed_streams_.end()) {
    return nullptr;
  }

  DCHECK(url.SchemeIsCryptographic());
  for (SessionList::iterator it = url_it->second.begin();
       it != url_it->second.end();) {
    SpdySession* spdy_session = *it;
    ++it;
    const SpdySessionKey& spdy_session_key = spdy_session->spdy_session_key();
    if (spdy_session_key.proxy_server() != key.proxy_server() ||
        spdy_session_key.privacy_mode() != key.privacy_mode()) {
      continue;
    }
    if (!spdy_session->VerifyDomainAuthentication(
            key.host_port_pair().host())) {
      continue;
    }
    return spdy_session;
  }
  if (url_it->second.empty()) {
    unclaimed_pushed_streams_.erase(url_it);
  }

  return nullptr;
}

void Http2PushPromiseIndex::RegisterUnclaimedPushedStream(
    const GURL& url,
    SpdySession* spdy_session) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  // Use lower_bound() so that if key does not exists, then insertion can use
  // its return value as a hint.
  UnclaimedPushedStreamMap::iterator url_it =
      unclaimed_pushed_streams_.lower_bound(url);
  if (url_it == unclaimed_pushed_streams_.end() || url_it->first != url) {
    unclaimed_pushed_streams_.insert(
        url_it, std::make_pair(url, SessionList{spdy_session}));
    return;
  }
  url_it->second.push_back(spdy_session);
}

void Http2PushPromiseIndex::UnregisterUnclaimedPushedStream(
    const GURL& url,
    SpdySession* spdy_session) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  UnclaimedPushedStreamMap::iterator url_it =
      unclaimed_pushed_streams_.find(url);
  DCHECK(url_it != unclaimed_pushed_streams_.end());
  for (SessionList::iterator it = url_it->second.begin();
       it != url_it->second.end();) {
    if (*it != spdy_session) {
      continue;
    }
    url_it->second.erase(it);
    if (url_it->second.empty()) {
      unclaimed_pushed_streams_.erase(url_it);
    }
    return;
  }
  NOTREACHED();
}

}  // namespace net
