// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/chromium/http2_push_promise_index.h"
#include "base/trace_event/memory_usage_estimator.h"

#include <utility>

namespace net {

Http2PushPromiseIndex::Http2PushPromiseIndex() = default;

Http2PushPromiseIndex::~Http2PushPromiseIndex() {
  DCHECK(unclaimed_pushed_streams_sorted_by_url_.empty());
  DCHECK(unclaimed_pushed_streams_sorted_by_delegate_.empty());
}

bool Http2PushPromiseIndex::RegisterUnclaimedPushedStream(
    const GURL& url,
    SpdyStreamId stream_id,
    Delegate* delegate) {
  DCHECK(!url.is_empty());
  DCHECK_GT(stream_id, kNoPushedStreamFound);
  DCHECK(delegate);

  // Find an entry with |url| for |delegate| is such exists.  It is okay to cast
  // away const from |delegate|, because it is only used for lookup.
  auto it = unclaimed_pushed_streams_sorted_by_delegate_.lower_bound(
      UnclaimedPushedStream{url, kNoPushedStreamFound,
                            const_cast<Delegate*>(delegate)});
  // If such entry is found, do not allow registering another one.
  if (it != unclaimed_pushed_streams_sorted_by_delegate_.end() &&
      it->url == url && it->delegate == delegate) {
    return false;
  }

  unclaimed_pushed_streams_sorted_by_delegate_.insert(
      it, UnclaimedPushedStream{url, stream_id, delegate});

  bool success;
  std::tie(std::ignore, success) =
      unclaimed_pushed_streams_sorted_by_url_.insert(
          UnclaimedPushedStream{url, stream_id, delegate});
  DCHECK(success);

  return true;
}

bool Http2PushPromiseIndex::UnregisterUnclaimedPushedStream(
    const GURL& url,
    SpdyStreamId stream_id,
    Delegate* delegate) {
  DCHECK(!url.is_empty());
  DCHECK_GT(stream_id, kNoPushedStreamFound);
  DCHECK(delegate);

  size_t result = unclaimed_pushed_streams_sorted_by_url_.erase(
      UnclaimedPushedStream{url, stream_id, delegate});

  // If entry was not registered, then
  // |unclaimed_pushed_streams_sorted_by_url_| has not changed.
  if (result == 0)
    return false;

  DCHECK_EQ(1u, result);

  result = unclaimed_pushed_streams_sorted_by_delegate_.erase(
      UnclaimedPushedStream{url, stream_id, delegate});

  // If entry was in |unclaimed_pushed_streams_sorted_by_url_|, then it must
  // have been in |unclaimed_pushed_streams_sorted_by_delegate_| as well.
  DCHECK_EQ(1u, result);

  return true;
}

size_t Http2PushPromiseIndex::CountStreamsForSession(
    const Delegate* delegate) const {
  DCHECK(delegate);

  // Find the first entry for |delegate|.  It is okay to cast away const from
  // |delegate|, because it is only used for lookup.
  auto it = unclaimed_pushed_streams_sorted_by_delegate_.lower_bound(
      UnclaimedPushedStream{GURL(), kNoPushedStreamFound,
                            const_cast<Delegate*>(delegate)});
  size_t count = 0;
  while (it != unclaimed_pushed_streams_sorted_by_delegate_.end() &&
         it->delegate == delegate) {
    ++count;
    ++it;
  }

  return count;
}

SpdyStreamId Http2PushPromiseIndex::FindStream(const GURL& url,
                                               const Delegate* delegate) const {
  // Find the entry for |delegate| that has |url| (there can be at most one such
  // entry).  It is okay to cast away const from |delegate|, because it is only
  // used for lookup.
  auto it = unclaimed_pushed_streams_sorted_by_delegate_.lower_bound(
      UnclaimedPushedStream{url, kNoPushedStreamFound,
                            const_cast<Delegate*>(delegate)});

  if (it == unclaimed_pushed_streams_sorted_by_delegate_.end() ||
      it->url != url || it->delegate != delegate) {
    return kNoPushedStreamFound;
  }

  return it->stream_id;
}

void Http2PushPromiseIndex::FindSession(const SpdySessionKey& key,
                                        const GURL& url,
                                        base::WeakPtr<SpdySession>* session,
                                        SpdyStreamId* stream_id) const {
  DCHECK(!url.is_empty());

  *session = nullptr;
  *stream_id = kNoPushedStreamFound;

  // Do not allow cross-origin push for non-cryptographic schemes.
  if (!url.SchemeIsCryptographic())
    return;

  auto it = unclaimed_pushed_streams_sorted_by_url_.lower_bound(
      UnclaimedPushedStream{url, kNoPushedStreamFound, nullptr});

  while (it != unclaimed_pushed_streams_sorted_by_url_.end() &&
         it->url == url) {
    if (it->delegate->ValidatePushedStream(key)) {
      *session = it->delegate->GetWeakPtrToSession();
      *stream_id = it->stream_id;
      it->delegate->OnPushedStreamClaimed(it->url, it->stream_id);
      return;
    }
    ++it;
  }
}

size_t Http2PushPromiseIndex::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(
             unclaimed_pushed_streams_sorted_by_url_) +
         base::trace_event::EstimateMemoryUsage(
             unclaimed_pushed_streams_sorted_by_delegate_);
}

size_t Http2PushPromiseIndex::UnclaimedPushedStream::EstimateMemoryUsage()
    const {
  return base::trace_event::EstimateMemoryUsage(url) + sizeof(SpdyStreamId) +
         sizeof(Delegate*);
}

bool Http2PushPromiseIndex::CompareByUrl::operator()(
    const UnclaimedPushedStream& a,
    const UnclaimedPushedStream& b) const {
  // Compare by URL first.
  if (a.url < b.url)
    return true;
  if (a.url > b.url)
    return false;
  // For identical URL, put kNoPushedStreamFound first.
  if (a.stream_id == kNoPushedStreamFound &&
      b.stream_id != kNoPushedStreamFound) {
    return true;
  }
  if (a.stream_id != kNoPushedStreamFound &&
      b.stream_id == kNoPushedStreamFound) {
    return false;
  }
  // Then compare by stream ID.
  if (a.stream_id < b.stream_id)
    return true;
  if (a.stream_id > b.stream_id)
    return false;
  // If URL and stream ID are identical, compare by Delegate.
  return a.delegate < b.delegate;
}

bool Http2PushPromiseIndex::CompareByDelegate::operator()(
    const UnclaimedPushedStream& a,
    const UnclaimedPushedStream& b) const {
  // Compare by Delegate first.
  if (a.delegate < b.delegate)
    return true;
  if (a.delegate > b.delegate)
    return false;
  // For identical Delegate, put empty URL first.
  if (a.url.is_empty() && !b.url.is_empty())
    return true;
  if (!a.url.is_empty() && a.url.is_empty())
    return false;
  // Then compare by URL.
  if (a.url < b.url)
    return true;
  if (a.url > b.url)
    return false;
  // If Delegate and URL are identical, put kNoPushedStreamFound first.
  if (a.stream_id == kNoPushedStreamFound &&
      b.stream_id != kNoPushedStreamFound) {
    return true;
  }
  if (a.stream_id != kNoPushedStreamFound &&
      b.stream_id == kNoPushedStreamFound) {
    return false;
  }
  // Otherwise, compare by stream ID.
  return a.stream_id < b.stream_id;
}

}  // namespace net
