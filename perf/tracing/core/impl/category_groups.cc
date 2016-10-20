// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "perf/tracing/core/impl/category_groups.h"

#include <string.h>

#include "perf/tracing/core/impl/auto_lock.h"

namespace tracing {
namespace internal {

using ::tracing::platform::DCHECK;

namespace {
constexpr size_t kMaxCategoryGroups = 200;

// |g_category_groups| and |g_category_group_enabled| are parallel arrays. This
// is to easily convert a pointer to a member of g_category_group_enabled to
// the corresponding category group name in |g_category_groups|.
const char* g_category_groups[kMaxCategoryGroups] = {
    "toplevel", "tracing already shutdown",
    "tracing categories exhausted; must increase MAX_CATEGORY_GROUPS",
    "__metadata"};

unsigned char g_category_group_enabled[kMaxCategoryGroups] = {0};

// Indexes here have to match the g_category_groups array indexes above.
const int kCategoryAlreadyShutdown = 1;
const int kCategoryCategoriesExhausted = 2;
const int kCategoryMetadata = 3;
const int kNumBuiltinCategories = 4;

// Skip default categories.
tracing::platform::AtomicWord g_category_index = kNumBuiltinCategories;

}  // namespace

const unsigned char* GetCategoryGroupEnabledSlow(const char* category_group) {
  DCHECK(!strchr(category_group, '"'))
      << "Category groups may not contain double quote";

  // |g_category_groups| is append only, try a first lookup without a lock.
  size_t current_category_index = platform::Acquire_Load(&g_category_index);
  for (size_t i = 0; i < current_category_index; ++i) {
    if (strcmp(g_category_groups[i], category_group) == 0) {
      return &g_category_group_enabled[i];
    }
  }

  // This is the slow path: the lock is not held in the case above, so more
  // than one thread could have reached here trying to add the same category.
  // Only hold to lock when actually appending a new category, and
  // check the categories groups again.
  AutoLock lock(lock_);
  size_t category_index = tracing::platform::Acquire_Load(&g_category_index);
  for (size_t i = 0; i < category_index; ++i) {
    if (strcmp(g_category_groups[i], category_group) == 0) {
      return &g_category_group_enabled[i];
    }
  }

  // Create a new category group.

  if (category_index >= kMaxCategoryGroups) {
    NOTREACHED() << "Reached max number of tracing categories. Increase "
                    "kMaxCategoryGroups.";
    return &g_category_group_enabled[kCategoryCategoriesExhausted];
  }

  // Don't hold on to the category_group pointer, so that we can create
  // category groups with strings not known at compile time (this is
  // required by SetWatchEvent).
  const char* new_group = strdup(category_group);
  ANNOTATE_LEAKING_OBJECT_PTR(new_group);
  g_category_groups[category_index] = new_group;
  DCHECK(!g_category_group_enabled[category_index]);

  // Note that if both included and excluded patterns in the
  // TraceConfig are empty, we exclude nothing,
  // thereby enabling this category group.
  UpdateCategoryGroupEnabledFlag(category_index);
  category_group_enabled = &g_category_group_enabled[category_index];

  // Update the max index now.
  tracing::platform::Release_Store(&g_category_index, category_index + 1);
}

}  // namespace internal
}  // namespace tracing
