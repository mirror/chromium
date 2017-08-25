// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_NEW_GENERATION_FEATURES_H
#define COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_NEW_GENERATION_FEATURES_H

#include "base/feature_list.h"
#include "build/build_config.h"

namespace bookmark_new_generation {
namespace features {

#if defined(OS_IOS)
// Used to control the state of the Credential Manager API feature.
extern const base::Feature kBookmarkNewGeneration;
#endif

}  // namespace features
}  // namespace bookmark_new_generation

#endif  // COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_NEW_GENERATION_FEATURES_H
