// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_cached_areas.h"

#include "content/public/renderer/render_frame.h"
#include "content/renderer/dom_storage/local_storage_cached_area.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {

LocalStorageCachedAreas::LocalStorageCachedAreas() {}

LocalStorageCachedAreas::~LocalStorageCachedAreas() {}

scoped_refptr<LocalStorageCachedArea> LocalStorageCachedAreas::GetCachedArea(
    blink::WebLocalFrame* web_frame) {
  url::Origin origin = web_frame->GetDocument().GetSecurityOrigin();
  if (cached_areas_.find(origin) == cached_areas_.end())
    cached_areas_[origin] = new LocalStorageCachedArea(web_frame, this);

  return make_scoped_refptr(cached_areas_[origin]);
}

void LocalStorageCachedAreas::CacheAreaClosed(
    LocalStorageCachedArea* cached_area) {
  DCHECK(cached_areas_.find(cached_area->origin()) != cached_areas_.end());
  cached_areas_.erase(cached_area->origin());
}

}  // namespace content
