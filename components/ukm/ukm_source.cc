// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/ukm_source.h"

#include "base/atomicops.h"
#include "base/hash.h"
#include "components/metrics/proto/ukm/source.pb.h"

namespace ukm {

namespace {

// The maximum length of a URL we will record.
constexpr int kMaxURLLength = 2 * 1024;

// The string sent in place of a URL if the real URL was too long.
constexpr char kMaxUrlLengthMessage[] = "URLTooLong";

// Returns a URL that is under the length limit, by returning a constant
// string when the URl is too long.
std::string GetShortenedURL(const GURL& url) {
  if (url.spec().length() > kMaxURLLength)
    return kMaxUrlLengthMessage;
  return url.spec();
}

#if defined(OS_ANDROID)
base::subtle::Atomic32 g_custom_tab = -1;
#endif

}  // namespace

#if defined(OS_ANDROID)
// static
void UmkSource::SetCustomTabVisible(bool visible) {
  base::subtle::NoBarrier_Store(&g_custom_tab, visible ? 1 : 0);
}
#endif

UkmSource::UkmSource()
#if defined(OS_ANDROID)
    : custom_tab_(base::subtle::NoBarrier_Load(&g_custom_tab))
#endif
{
}

UkmSource::~UkmSource() = default;

void UkmSource::UpdateUrl(const GURL& url) {
  DCHECK(!url_.is_empty());
  if (url_ == url)
    return;
  if (initial_url_.is_empty())
    initial_url_ = url_;
  url_ = url;
}

void UkmSource::PopulateProto(Source* proto_source) const {
  DCHECK(!proto_source->has_id());
  DCHECK(!proto_source->has_url());
  DCHECK(!proto_source->has_initial_url());

  proto_source->set_id(id_);
  proto_source->set_url(GetShortenedURL(url_));
  if (!initial_url_.is_empty())
    proto_source->set_initial_url(GetShortenedURL(initial_url_));

#if defined(OS_ANDROID)
  if (custom_tab_ >= 0)
    proto_source->set_custom_tab(!!custom_tab_);
#endif
}

}  // namespace ukm
