// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"

#include <utility>

namespace net {

Http2PushPromiseIndex::Http2PushPromiseIndex() {}

Http2PushPromiseIndex::~Http2PushPromiseIndex() {
  DCHECK(entries_.empty());
}

base::WeakPtr<SpdySession> Http2PushPromiseIndex::Find(
    const SpdySessionKey& key,
    const GURL& url) const {
  DCHECK(!url.is_empty());

  EntryMap::const_iterator it = entries_.find(url);

  if (it == entries_.end()) {
    return base::WeakPtr<SpdySession>();
  }

  DCHECK(url.SchemeIsCryptographic());
  for (const auto& entry : it->second) {
    if (entry->Validate(key)) {
      return entry->spdy_session();
    }
  }

  return base::WeakPtr<SpdySession>();
}

void Http2PushPromiseIndex::Register(const GURL& url,
                                     std::unique_ptr<Entry> entry) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  // Use lower_bound() so that if key does not exists, then insertion can use
  // its return value as a hint.
  EntryMap::iterator url_it = entries_.lower_bound(url);
  if (url_it != entries_.end() && url_it->first == url) {
    url_it->second.push_back(std::move(entry));
    return;
  }
  EntryList list;
  list.push_back(std::move(entry));
  EntryMap::value_type value(url, std::move(list));
  entries_.insert(url_it, std::move(value));
}

void Http2PushPromiseIndex::Unregister(const GURL& url, const Entry& entry) {
  DCHECK(!url.is_empty());
  DCHECK(url.SchemeIsCryptographic());

  EntryMap::iterator url_it = entries_.find(url);
  if (url_it == entries_.end()) {
    NOTREACHED() << "Only a previously registered entry can be unregistered.";
    return;
  }
  for (EntryList::iterator it = url_it->second.begin();
       it != url_it->second.end();) {
    if ((*it)->spdy_session().get() == entry.spdy_session().get()) {
      url_it->second.erase(it);
      if (url_it->second.empty()) {
        entries_.erase(url_it);
      }
      return;
    }
    ++it;
  }
  NOTREACHED();
}

}  // namespace net
