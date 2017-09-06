// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip_layout.h"

#include <stddef.h>

#include <algorithm>

#include "base/logging.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/gfx/geometry/rect.h"

namespace {

// Calculates the size for normal tabs.
// |is_active_tab_normal| is true if the active tab is a normal tab, if false
// the active tab is not in the set of normal tabs.
// |normal_width| is the available width for normal tabs.
void CalculateNormalTabWidths(const TabSizeInfo& tab_size_info,
                              bool is_active_tab_normal,
                              int num_normal_tabs,
                              int normal_width,
                              int* active_width,
                              int* inactive_width,
                              float* endcap_width) {
  DCHECK_NE(0, num_normal_tabs);

  const int num_inactive_tabs = num_normal_tabs - 1;

  const int min_inactive_width = tab_size_info.min_inactive_width;
  *inactive_width = min_inactive_width;
  *active_width = tab_size_info.min_active_width;
  const int max_width = tab_size_info.max_size.width();
  const float width_range = max_width - min_inactive_width;

  const float max_endcap_width = TabStrip::GetTabEndcapMaxWidth();
  const float min_endcap_width = Tab::kMinimumEndcapWidth;
  const float endcap_width_range = max_endcap_width - min_endcap_width;

  float desired_tab_width =
      width_range * (normal_width + num_inactive_tabs * min_endcap_width) -
      ((min_inactive_width)*endcap_width_range) * num_inactive_tabs;
  desired_tab_width /=
      static_cast<float>(num_normal_tabs * (width_range - endcap_width_range) +
                         endcap_width_range);
  if (desired_tab_width >= max_width) {
    desired_tab_width = max_width;
    *endcap_width = max_endcap_width;
    *inactive_width = gfx::ToRoundedInt(desired_tab_width);
    *active_width = gfx::ToRoundedInt(desired_tab_width);
    return;
  }

  *inactive_width =
      std::max(gfx::ToRoundedInt(desired_tab_width), min_inactive_width);
  *active_width = std::max(gfx::ToRoundedInt(desired_tab_width),
                           tab_size_info.min_active_width);

  const float tab_width_ratio =
      static_cast<float>(*inactive_width - min_inactive_width) /
      static_cast<float>(max_width - min_inactive_width);

  *endcap_width = min_endcap_width + tab_width_ratio * endcap_width_range;
  // Calculate the desired tab widths by dividing the available space into equal
  // portions.  Don't let tabs get larger than the "standard width" or smaller
  // than the minimum width for each type, respectively.
  const int total_overlap =
      gfx::ToRoundedInt(*endcap_width) * num_inactive_tabs;

  // |desired_tab_width| was calculated assuming the active and inactive tabs
  // get the same width. If this isn't the case, then we need to recalculate
  // the size for inactive tabs based on how big the active tab is.
  if (*inactive_width != *active_width && is_active_tab_normal &&
      num_normal_tabs > 1) {
    *inactive_width = std::max(
        (normal_width + total_overlap - *active_width) / num_inactive_tabs,
        min_inactive_width);
  }
}

}  // namespace

void CalculateBoundsForPinnedTabs(const TabSizeInfo& tab_size_info,
                                  int num_pinned_tabs,
                                  int num_tabs,
                                  std::vector<gfx::Rect>* tabs_bounds) {
  DCHECK_EQ(static_cast<size_t>(num_tabs), tabs_bounds->size());
  int index = 0;
  int next_x = 0;
  for (; index < num_pinned_tabs; ++index) {
    (*tabs_bounds)[index].SetRect(next_x, 0, tab_size_info.pinned_tab_width,
                                  tab_size_info.max_size.height());
    next_x += tab_size_info.pinned_tab_width - tab_size_info.tab_overlap;
  }
  if (index > 0 && index < num_tabs) {
    (*tabs_bounds)[index].set_x(next_x + tab_size_info.pinned_to_normal_offset);
  }
}

std::vector<gfx::Rect> CalculateBounds(const TabSizeInfo& tab_size_info,
                                       int num_pinned_tabs,
                                       int num_tabs,
                                       int active_index,
                                       int width,
                                       int* active_width,
                                       int* inactive_width,
                                       float* endcap_width) {
  DCHECK_NE(0, num_tabs);

  std::vector<gfx::Rect> tabs_bounds(num_tabs);

  *active_width = *inactive_width = tab_size_info.max_size.width();
  *endcap_width = TabStrip::GetTabEndcapMaxWidth();

  int next_x = 0;
  if (num_pinned_tabs) {
    CalculateBoundsForPinnedTabs(tab_size_info, num_pinned_tabs, num_tabs,
                                 &tabs_bounds);
    if (num_pinned_tabs == num_tabs)
      return tabs_bounds;

    // CalculateBoundsForPinnedTabs() sets the x location of the first normal
    // tab.
    width -= tabs_bounds[num_pinned_tabs].x();
    next_x = tabs_bounds[num_pinned_tabs].x();
  }

  const bool is_active_tab_normal = active_index >= num_pinned_tabs;
  const int num_normal_tabs = num_tabs - num_pinned_tabs;
  CalculateNormalTabWidths(tab_size_info, is_active_tab_normal, num_normal_tabs,
                           width, active_width, inactive_width, endcap_width);

  // As CalculateNormalTabWidths() calculates sizes using ints there may be a
  // bit of extra space (due to the available width not being an integer
  // multiple of these sizes). Give the extra space to the first tabs, and only
  // give extra space to the active tab if it is the same size as the inactive
  // tabs (the active tab may already be bigger).
  int expand_width_count = 0;
  bool give_extra_space_to_active = false;
  int adjust = 1;
  if (*inactive_width != tab_size_info.max_size.width()) {
    give_extra_space_to_active = *active_width == *inactive_width;
    expand_width_count =
        width - ((*inactive_width - gfx::ToRoundedInt(*endcap_width)) *
                 (num_normal_tabs - 1));
    expand_width_count -=
        (is_active_tab_normal ? *active_width : *inactive_width);
    if (expand_width_count < 0)
      adjust = -1;
  }

  if (*inactive_width == tab_size_info.min_inactive_width &&
      expand_width_count < 0) {
    // Do not decrease width if tab is already at minimum width.
    expand_width_count = 0;
  }
  // Set the ideal bounds of the normal tabs.
  // GenerateIdealBoundsForPinnedTabs() set the ideal bounds of the pinned tabs.
  for (int i = num_pinned_tabs; i < num_tabs; ++i) {
    const bool is_active = i == active_index;
    int width = is_active ? *active_width : *inactive_width;
    if (expand_width_count != 0 && (!is_active || give_extra_space_to_active)) {
      width += adjust;
      expand_width_count -= adjust;
      if (i + std::abs(expand_width_count) > num_normal_tabs) {
        width += adjust;
        expand_width_count -= adjust;
      }
    }
    tabs_bounds[i].SetRect(next_x, 0, width, tab_size_info.max_size.height());
    next_x += tabs_bounds[i].width() - gfx::ToRoundedInt(*endcap_width);
  }

  return tabs_bounds;
}
