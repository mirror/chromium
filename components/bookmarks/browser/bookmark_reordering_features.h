// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_REORDERING_FEATURES_H
#define COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_REORDERING_FEATURES_H

#include "base/feature_list.h"
#include "build/build_config.h"

namespace bookmark_reordering {
namespace features {

#if defined(OS_IOS)
// Used to control the state of the Credential Manager API feature.
extern const base::Feature kBookmarkReordering;
#endif

}  // namespace features
}  // namespace bookmark_reordering

#endif  // COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_REORDERING_FEATURES_H
