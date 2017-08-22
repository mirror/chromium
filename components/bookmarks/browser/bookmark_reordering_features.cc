// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/browser/bookmark_reordering_features.h"

namespace bookmark_reordering {
namespace features {

#if defined(OS_IOS)
const base::Feature kBookmarkReordering{"BookmarkReordering",
                                        base::FEATURE_DISABLED_BY_DEFAULT};
#endif

}  // namespace features
}  // namespace bookmark_reordering
