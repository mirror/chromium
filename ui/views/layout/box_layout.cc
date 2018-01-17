// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/layout/box_layout.h"

#include <algorithm>
#include <fstream>
#include <numeric>

#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "ui/views/view_properties.h"

namespace views {

namespace {

gfx::Insets GetMarginsForView(const View* v) {
  gfx::Insets* margins = v ? v->GetProperty(kMarginsKey) : nullptr;
  return margins ? *margins : gfx::Insets();
}

// Returns the maximum of the given insets along the given |axis|.
// NOTE: |axis| is different from |orientation_|; it specifies the actual
// desired axis.
enum Axis { HORIZONTAL_AXIS, VERTICAL_AXIS };

gfx::Insets MaxAxisInsets(Axis axis,
                          const gfx::Insets& leading1,
                          const gfx::Insets& leading2,
                          const gfx::Insets& trailing1,
                          const gfx::Insets& trailing2) {
  if (axis == HORIZONTAL_AXIS) {
    return gfx::Insets(0, std::max(leading1.left(), leading2.left()), 0,
                       std::max(trailing1.right(), trailing2.right()));
  }
  return gfx::Insets(std::max(leading1.top(), leading2.top()), 0,
                     std::max(trailing1.bottom(), trailing2.bottom()), 0);
}

}  // namespace

BoxLayout::ViewWrapper::ViewWrapper() : view_(nullptr), layout_(nullptr) {}

BoxLayout::ViewWrapper::ViewWrapper(const BoxLayout* layout, View* view)
    : view_(view), layout_(layout) {
  margins_ = GetMarginsForView(view);

  // Since we are aligning to content view edges, remove the cross axis margin
  // when laying out.
  sizing_margins_ = layout_->orientation_ == Orientation::kHorizontal
                        ? gfx::Insets(0, margins_.left(), 0, margins_.right())
                        : gfx::Insets(margins_.top(), 0, margins_.bottom(), 0);
}

BoxLayout::ViewWrapper::~ViewWrapper() {}

int BoxLayout::ViewWrapper::GetHeightForWidth(int width) const {
  // When collapse_margins_spacing_ is true, the BoxLayout handles the margin
  // calculations because it has to compare and use only the largest of several
  // adjacent margins or border insets.
  if (layout_->collapse_margins_spacing_)
    return view_->GetHeightForWidth(width);
  // When collapse_margins_spacing_ is false, the view margins are included in
  // the "virtual" size of the view. The view itself is unaware of this, so this
  // information has to be excluded before the call to View::GetHeightForWidth()
  // and added back in to the result.
  // If the orientation_ is kVertical, the cross-axis is the actual view width.
  // This is because the cross-axis margins are always handled by the layout.
  if (layout_->orientation_ == Orientation::kHorizontal) {
    return view_->GetHeightForWidth(
               std::max(0, width - sizing_margins_.width())) +
           sizing_margins_.height();
  }
  return view_->GetHeightForWidth(width) + sizing_margins_.height();
}

gfx::Size BoxLayout::ViewWrapper::GetPreferredSize() const {
  gfx::Size preferred_size = view_->GetPreferredSize();
  if (!layout_->collapse_margins_spacing_)
    preferred_size.Enlarge(sizing_margins_.width(), sizing_margins_.height());
  return preferred_size;
}

void BoxLayout::ViewWrapper::SetBoundsRect(const gfx::Rect& bounds) {
  gfx::Rect new_bounds = bounds;

  if (!layout_->collapse_margins_spacing_) {
    if (layout_->orientation_ == Orientation::kHorizontal) {
      new_bounds.set_x(bounds.x() + margins_.left());
      new_bounds.set_width(
          std::max(0, bounds.width() - sizing_margins_.width()));
    } else {
      new_bounds.set_y(bounds.y() + margins_.top());
      new_bounds.set_height(
          std::max(0, bounds.height() - sizing_margins_.height()));
    }
  }

  view_->SetBoundsRect(new_bounds);
}

bool BoxLayout::ViewWrapper::visible() const {
  return view_->visible();
}

BoxLayout::BoxLayout(BoxLayout::Orientation orientation,
                     const gfx::Insets& inside_border_insets,
                     int between_child_spacing,
                     bool collapse_margins_spacing)
    : orientation_(orientation),
      inside_border_insets_(inside_border_insets),
      between_child_spacing_(between_child_spacing),
      main_axis_alignment_(MAIN_AXIS_ALIGNMENT_START),
      cross_axis_alignment_(CROSS_AXIS_ALIGNMENT_STRETCH),
      default_flex_(0),
      minimum_cross_axis_size_(0),
      collapse_margins_spacing_(collapse_margins_spacing),
      host_(nullptr) {}

BoxLayout::~BoxLayout() {
}

void BoxLayout::SetFlexForView(const View* view, int flex_weight) {
  DCHECK(host_);
  DCHECK(view);
  DCHECK_EQ(host_, view->parent());
  DCHECK_GE(flex_weight, 0);
  flex_map_[view] = flex_weight;
}

void BoxLayout::ClearFlexForView(const View* view) {
  DCHECK(view);
  flex_map_.erase(view);
}

void BoxLayout::SetDefaultFlex(int default_flex) {
  DCHECK_GE(default_flex, 0);
  default_flex_ = default_flex;
}

void BoxLayout::ConstrainToCrossAxisSize(int child_area_cross_axis_size,
                                         std::vector<gfx::Rect>* child_bounds) {
  for (gfx::Rect& child : *child_bounds) {
    SetCrossAxisSize(std::min(child_area_cross_axis_size, CrossAxisSize(child)),
                     &child);
  }
}

void BoxLayout::ConstrainToMainAxisSize(int child_area_main_axis_size,
                                        std::vector<gfx::Rect>* child_bounds) {
  for (gfx::Rect& child : *child_bounds) {
    SetMainAxisSize(
        std::min(child_area_main_axis_size - MainAxisPosition(child),
                 MainAxisSize(child)),
        &child);
  }
}

void BoxLayout::Layout(View* host) {
  DCHECK_EQ(host_, host);
  gfx::Rect child_area(host->GetContentsBounds());

  if (child_area.IsEmpty())
    return;

  std::vector<View*> children;
  for (int i = 0; i < host->child_count(); ++i)
    children.push_back(host->child_at(i));

  if (!std::any_of(children.begin(), children.end(),
                   [](View* v) { return v->visible(); })) {
    return;
  }

  AdjustMainAxisForMargin(&child_area);
  if (!collapse_margins_spacing_) {
    AdjustCrossAxisForInsets(&child_area);
  }

  std::vector<gfx::Rect> child_bounds = CalculateChildPreferredSizes();

  int flex_sum = std::accumulate(
      children.begin(), children.end(), 0,
      [this](int sum, auto child) { return sum + GetFlexForView(child); });

  int total_main_axis_size = 0;
  int main_free_space = 0;
  int children_center_line =
      cross_axis_alignment_ == CROSS_AXIS_ALIGNMENT_CENTER
          ? CalculateIdealCenterLineForChildren()
          : 0;
  gfx::Insets max_cross_axis_margin = CrossAxisMaxViewMargin();

  // Heights are dependent on widths, so the width must always be calculated
  // before the height. All calculations are done relative to the child area.
  if (orientation_ == kHorizontal) {
    // Calculate the total width of all children and horizontal paddings along
    // the main axis, and then use the remaining space to apply flex, completing
    // our width calculations.
    total_main_axis_size = GetTotalMainAxisSize(child_bounds);
    main_free_space = MainAxisSize(child_area) - total_main_axis_size;
    ApplyFlex(main_free_space, flex_sum, &child_bounds);
    CalculateMainAxisPositions(total_main_axis_size, &child_bounds);

    // Clip views along the main axis first, as this will influence height.
    ConstrainToMainAxisSize(MainAxisSize(child_area), &child_bounds);

    // Calculate heights.
    if (cross_axis_alignment_ == CROSS_AXIS_ALIGNMENT_STRETCH) {
      ApplyStretch(child_area, &child_bounds);
    } else {
      UpdateHeightsForWidth(&child_bounds);
    }
    CalculateCrossAxisPositions(CrossAxisSize(child_area),
                                max_cross_axis_margin, children_center_line,
                                &child_bounds);
  } else {
    // Calculate widths.
    if (cross_axis_alignment_ == CROSS_AXIS_ALIGNMENT_STRETCH)
      ApplyStretch(child_area, &child_bounds);

    // Clip views along the cross axis first, as this will influence height.
    CalculateCrossAxisPositions(CrossAxisSize(child_area),
                                max_cross_axis_margin, children_center_line,
                                &child_bounds);
    ConstrainToCrossAxisSize(CrossAxisSize(child_area), &child_bounds);
    UpdateHeightsForWidth(&child_bounds);

    // Calculate heights.
    total_main_axis_size = GetTotalMainAxisSize(child_bounds);
    main_free_space = MainAxisSize(child_area) - total_main_axis_size;
    ApplyFlex(main_free_space, flex_sum, &child_bounds);
    CalculateMainAxisPositions(total_main_axis_size, &child_bounds);
  }

  int main_axis_offset =
      GetMainAxisOffset(flex_sum, total_main_axis_size, main_free_space);
  for (int i = 0; i < host->child_count(); ++i) {
    ViewWrapper child(this, host_->child_at(i));
    gfx::Rect bounds;
    SetMainAxisPosition(MainAxisPosition(child_bounds[i]) +
                            MainAxisPosition(child_area) + main_axis_offset,
                        &bounds);
    SetMainAxisSize(MainAxisSize(child_bounds[i]), &bounds);
    SetCrossAxisPosition(
        CrossAxisPosition(child_bounds[i]) + CrossAxisPosition(child_area),
        &bounds);
    SetCrossAxisSize(CrossAxisSize(child_bounds[i]), &bounds);

    // Clip to the host so that clipping ignores margins.
    bounds.Intersect(host->bounds());
    child.SetBoundsRect(bounds.IsEmpty() ? gfx::Rect() : bounds);
  }
}

std::vector<gfx::Rect> BoxLayout::CalculateChildPreferredSizes() const {
  std::vector<gfx::Rect> preferred_sizes;
  for (int i = 0; i < host_->child_count(); ++i) {
    ViewWrapper v(this, host_->child_at(i));
    preferred_sizes.push_back(
        gfx::Rect(v.visible() ? v.GetPreferredSize() : gfx::Size()));
  }
  return preferred_sizes;
}

void BoxLayout::ApplyStretch(const gfx::Rect& child_area,
                             std::vector<gfx::Rect>* child_bounds) {
  gfx::Rect cross_axis_area(child_area);
  cross_axis_area.Inset(CrossAxisMaxViewMargin());

  for (auto& b : *child_bounds)
    SetCrossAxisSize(CrossAxisSize(cross_axis_area), &b);
}

void BoxLayout::CalculateMainAxisPositions(
    int total_main_axis_size,
    std::vector<gfx::Rect>* child_bounds) {
  int main_position = 0;
  for (int i = 0; i < host_->child_count(); ++i) {
    ViewWrapper child(this, host_->child_at(i));
    SetMainAxisPosition(main_position, &child_bounds->at(i));
    if (MainAxisSize(child_bounds->at(i)) > 0 ||
        GetFlexForView(child.view()) > 0)
      main_position += MainAxisSize(child_bounds->at(i)) +
                       MainAxisMarginBetweenViews(
                           child, ViewWrapper(this, NextVisibleView(i)));
  }
}

int BoxLayout::GetMainAxisOffset(int flex_sum,
                                 int total_main_axis_size,
                                 int main_free_space) {
  int offset = 0;

  if (flex_sum)
    return offset;

  switch (main_axis_alignment_) {
    case MAIN_AXIS_ALIGNMENT_START:
      break;
    case MAIN_AXIS_ALIGNMENT_CENTER:
      offset = main_free_space / 2;
      break;
    case MAIN_AXIS_ALIGNMENT_END:
      offset = main_free_space;
      break;
    default:
      NOTREACHED();
  }

  return std::max(0, offset);
}

int BoxLayout::GetTotalMainAxisSize(
    const std::vector<gfx::Rect>& child_bounds) const {
  int total_main_axis_size = -between_child_spacing_;
  for (int i = 0; i < host_->child_count(); ++i) {
    const ViewWrapper child(this, host_->child_at(i));
    if (!child.visible())
      continue;

    int flex = GetFlexForView(child.view());
    int child_main_axis_size = MainAxisSize(child_bounds[i]);
    if (child_main_axis_size == 0 && flex == 0)
      continue;

    total_main_axis_size += child_main_axis_size +
                            MainAxisMarginBetweenViews(
                                child, ViewWrapper(this, NextVisibleView(i)));
  }
  return total_main_axis_size;
}

void BoxLayout::ApplyFlex(int main_free_space,
                          int flex_sum,
                          std::vector<gfx::Rect>* child_bounds) const {
  if (flex_sum == 0)
    return;

  int current_flex = 0;
  int total_padding = 0;
  for (int i = 0; i < host_->child_count(); i++) {
    ViewWrapper child(this, host_->child_at(i));

    // Calculate flex padding.
    int current_padding = 0;
    if (GetFlexForView(child.view()) > 0) {
      current_flex += GetFlexForView(child.view());
      int quot = (main_free_space * current_flex) / flex_sum;
      int rem = (main_free_space * current_flex) % flex_sum;
      current_padding = quot - total_padding;
      // Use the current remainder to round to the nearest pixel.
      if (std::abs(rem) * 2 >= flex_sum)
        current_padding += main_free_space > 0 ? 1 : -1;
      total_padding += current_padding;
    }

    SetMainAxisSize(MainAxisSize(child_bounds->at(i)) + current_padding,
                    &child_bounds->at(i));
  }

  DCHECK_EQ(total_padding, main_free_space);
}

void BoxLayout::UpdateHeightsForWidth(
    std::vector<gfx::Rect>* child_bounds) const {
  for (int i = 0; i < host_->child_count(); i++) {
    ViewWrapper child(this, host_->child_at(i));
    child_bounds->at(i).set_height(
        child.GetHeightForWidth(child_bounds->at(i).width()));
  }
}

int BoxLayout::CalculateIdealCenterLineForChildren() const {
  int max = 0;
  for (int i = 0; i < host_->child_count(); ++i) {
    View* child = host_->child_at(i);
    if (!child->visible())
      continue;

    int center = CrossAxisSize(gfx::Rect(child->GetPreferredSize())) / 2 +
                 CrossAxisLeadingInset(GetMarginsForView(child));
    max = std::max(max, center);
  }
  return max;
}

void BoxLayout::CalculateCrossAxisPositions(
    int child_area_cross_axis_size,
    const gfx::Insets& max_cross_axis_margin,
    int children_center_line,
    std::vector<gfx::Rect>* child_bounds) {
  for (int i = 0; i < host_->child_count(); i++) {
    ViewWrapper child(this, host_->child_at(i));
    gfx::Rect bounds = child_bounds->at(i);
    int view_cross_axis_size = CrossAxisSize(bounds);
    int free_space = child_area_cross_axis_size - view_cross_axis_size;
    int position = 0;
    switch (cross_axis_alignment_) {
      case CROSS_AXIS_ALIGNMENT_STRETCH:
      case CROSS_AXIS_ALIGNMENT_START:
        position = CrossAxisLeadingInset(max_cross_axis_margin);
        break;
      case CROSS_AXIS_ALIGNMENT_CENTER:
        position = std::max(free_space / 2,
                            children_center_line - CrossAxisSize(bounds) / 2);
        break;
      case CROSS_AXIS_ALIGNMENT_END:
        position = free_space - CrossAxisTrailingInset(max_cross_axis_margin);
        break;
      default:
        NOTREACHED();
    }

    SetCrossAxisPosition(position, &child_bounds->at(i));
  }
}

gfx::Size BoxLayout::GetPreferredSize(const View* host) const {
  DCHECK_EQ(host_, host);
  // Calculate the child views' preferred width.
  int width = 0;
  if (orientation_ == kVertical)
    width = GetCrossAxisPreferredSize();

  return GetPreferredSizeForChildWidth(host, width);
}

int BoxLayout::GetCrossAxisPreferredSize() const {
  auto max_value = [this](auto value_fn) {
    int max = 0;
    for (int i = 0; i < host_->child_count(); ++i) {
      View* child = host_->child_at(i);
      if (child->visible())
        max = std::max(max, value_fn(child));
    }

    return max;
  };
  gfx::Insets max_cross_axis_margins = CrossAxisMaxViewMargin();

  int cross_axis_size = 0;
  switch (cross_axis_alignment_) {
    case CROSS_AXIS_ALIGNMENT_STRETCH:
      cross_axis_size += CrossAxisLeadingInset(max_cross_axis_margins);
      cross_axis_size += max_value([this](View* v) {
        return CrossAxisSize(gfx::Rect(v->GetPreferredSize()));
      });
      cross_axis_size += CrossAxisTrailingInset(max_cross_axis_margins);
      break;
    case CROSS_AXIS_ALIGNMENT_START:
      cross_axis_size += max_value([this](View* v) {
        return CrossAxisLeadingInset(GetMarginsForView(v));
      });
      cross_axis_size += max_value([this](View* v) {
        return CrossAxisSize(gfx::Rect(v->GetPreferredSize())) +
               CrossAxisTrailingInset(GetMarginsForView(v));
      });
      break;
    case CROSS_AXIS_ALIGNMENT_CENTER:
      cross_axis_size += CalculateIdealCenterLineForChildren();
      cross_axis_size += max_value([this](View* v) {
        return (CrossAxisSize(gfx::Rect(v->GetPreferredSize())) + 1) / 2 +
               CrossAxisTrailingInset(GetMarginsForView(v));
      });
      break;
    case CROSS_AXIS_ALIGNMENT_END:
      cross_axis_size += max_value([this](View* v) {
        return CrossAxisLeadingInset(GetMarginsForView(v)) +
               CrossAxisSize(gfx::Rect(v->GetPreferredSize()));
      });
      cross_axis_size += max_value([this](View* v) {
        return CrossAxisTrailingInset(GetMarginsForView(v));
      });
      break;
    default:
      NOTREACHED();
  }
  return std::max(minimum_cross_axis_size_, cross_axis_size);
}

int BoxLayout::GetPreferredHeightForWidth(const View* host, int width) const {
  DCHECK_EQ(host_, host);
  int child_width = width - NonChildSize(host).width();
  return GetPreferredSizeForChildWidth(host, child_width).height();
}

void BoxLayout::Installed(View* host) {
  DCHECK(!host_);
  host_ = host;
}

void BoxLayout::ViewRemoved(View* host, View* view) {
  ClearFlexForView(view);
}

int BoxLayout::GetFlexForView(const View* view) const {
  FlexMap::const_iterator it = flex_map_.find(view);
  if (it == flex_map_.end())
    return default_flex_;

  return it->second;
}

int BoxLayout::MainAxisSize(const gfx::Rect& rect) const {
  return orientation_ == kHorizontal ? rect.width() : rect.height();
}

int BoxLayout::MainAxisPosition(const gfx::Rect& rect) const {
  return orientation_ == kHorizontal ? rect.x() : rect.y();
}

void BoxLayout::SetMainAxisSize(int size, gfx::Rect* rect) const {
  if (orientation_ == kHorizontal)
    rect->set_width(size);
  else
    rect->set_height(size);
}

void BoxLayout::SetMainAxisPosition(int position, gfx::Rect* rect) const {
  if (orientation_ == kHorizontal)
    rect->set_x(position);
  else
    rect->set_y(position);
}

int BoxLayout::CrossAxisSize(const gfx::Rect& rect) const {
  return orientation_ == kVertical ? rect.width() : rect.height();
}

int BoxLayout::CrossAxisPosition(const gfx::Rect& rect) const {
  return orientation_ == kVertical ? rect.x() : rect.y();
}

void BoxLayout::SetCrossAxisSize(int size, gfx::Rect* rect) const {
  if (orientation_ == kVertical)
    rect->set_width(size);
  else
    rect->set_height(size);
}

void BoxLayout::SetCrossAxisPosition(int position, gfx::Rect* rect) const {
  if (orientation_ == kVertical)
    rect->set_x(position);
  else
    rect->set_y(position);
}

int BoxLayout::MainAxisSizeForView(const ViewWrapper& view,
                                   int child_area_width) const {
  return orientation_ == kHorizontal
             ? view.GetPreferredSize().width()
             : view.GetHeightForWidth(cross_axis_alignment_ ==
                                              CROSS_AXIS_ALIGNMENT_STRETCH
                                          ? child_area_width
                                          : view.GetPreferredSize().width());
}

int BoxLayout::MainAxisLeadingInset(const gfx::Insets& insets) const {
  return orientation_ == kHorizontal ? insets.left() : insets.top();
}

int BoxLayout::MainAxisTrailingInset(const gfx::Insets& insets) const {
  return orientation_ == kHorizontal ? insets.right() : insets.bottom();
}

int BoxLayout::CrossAxisLeadingInset(const gfx::Insets& insets) const {
  return orientation_ == kVertical ? insets.left() : insets.top();
}

int BoxLayout::CrossAxisTrailingInset(const gfx::Insets& insets) const {
  return orientation_ == kVertical ? insets.right() : insets.bottom();
}

int BoxLayout::MainAxisMarginBetweenViews(const ViewWrapper& leading,
                                          const ViewWrapper& trailing) const {
  if (!collapse_margins_spacing_ || !leading.view() || !trailing.view())
    return between_child_spacing_;
  return std::max(between_child_spacing_,
                  std::max(MainAxisTrailingInset(leading.margins()),
                           MainAxisLeadingInset(trailing.margins())));
}

gfx::Insets BoxLayout::MainAxisOuterMargin() const {
  if (collapse_margins_spacing_) {
    const ViewWrapper first(this, FirstVisibleView());
    const ViewWrapper last(this, LastVisibleView());
    return MaxAxisInsets(
        orientation_ == kHorizontal ? HORIZONTAL_AXIS : VERTICAL_AXIS,
        inside_border_insets_, first.margins(), inside_border_insets_,
        last.margins());
  }
  return MaxAxisInsets(
      orientation_ == kHorizontal ? HORIZONTAL_AXIS : VERTICAL_AXIS,
      inside_border_insets_, gfx::Insets(), inside_border_insets_,
      gfx::Insets());
}

gfx::Insets BoxLayout::CrossAxisMaxViewMargin() const {
  int leading = 0;
  int trailing = 0;
  for (int i = 0; i < host_->child_count(); ++i) {
    const ViewWrapper child(this, host_->child_at(i));
    if (!child.visible())
      continue;
    leading = std::max(leading, CrossAxisLeadingInset(child.margins()));
    trailing = std::max(trailing, CrossAxisTrailingInset(child.margins()));
  }
  if (orientation_ == Orientation::kVertical)
    return gfx::Insets(0, leading, 0, trailing);
  return gfx::Insets(leading, 0, trailing, 0);
}

void BoxLayout::AdjustMainAxisForMargin(gfx::Rect* rect) const {
  rect->Inset(MainAxisOuterMargin());
}

void BoxLayout::AdjustCrossAxisForInsets(gfx::Rect* rect) const {
  rect->Inset(orientation_ == Orientation::kVertical
                  ? gfx::Insets(0, inside_border_insets_.left(), 0,
                                inside_border_insets_.right())
                  : gfx::Insets(inside_border_insets_.top(), 0,
                                inside_border_insets_.bottom(), 0));
}

gfx::Size BoxLayout::GetPreferredSizeForChildWidth(const View* host,
                                                   int child_area_width) const {
  DCHECK_EQ(host, host_);
  gfx::Rect child_area_bounds;

  std::vector<gfx::Rect> child_bounds = CalculateChildPreferredSizes();

  if (orientation_ == kHorizontal) {
    int total_main_axis_size = GetTotalMainAxisSize(child_bounds);
    child_area_bounds.set_width(total_main_axis_size);
    child_area_bounds.set_height(GetCrossAxisPreferredSize());
  } else {
    if (cross_axis_alignment_ == CROSS_AXIS_ALIGNMENT_STRETCH) {
      for (auto& b : child_bounds)
        b.set_width(child_area_width);

      UpdateHeightsForWidth(&child_bounds);
    }
    int total_main_axis_size = GetTotalMainAxisSize(child_bounds);

    child_area_bounds.set_width(child_area_width);
    child_area_bounds.set_height(total_main_axis_size);
  }

  gfx::Size non_child_size = NonChildSize(host_);
  return gfx::Size(child_area_bounds.width() + non_child_size.width(),
                   child_area_bounds.height() + non_child_size.height());
}

gfx::Size BoxLayout::NonChildSize(const View* host) const {
  gfx::Insets insets(host->GetInsets());
  if (!collapse_margins_spacing_)
    return gfx::Size(insets.width() + inside_border_insets_.width(),
                     insets.height() + inside_border_insets_.height());
  gfx::Insets main_axis = MainAxisOuterMargin();
  gfx::Insets cross_axis = inside_border_insets_;
  return gfx::Size(insets.width() + main_axis.width() + cross_axis.width(),
                   insets.height() + main_axis.height() + cross_axis.height());
}

View* BoxLayout::NextVisibleView(int index) const {
  for (int i = index + 1; i < host_->child_count(); ++i) {
    View* result = host_->child_at(i);
    if (result->visible())
      return result;
  }
  return nullptr;
}

View* BoxLayout::FirstVisibleView() const {
  return NextVisibleView(-1);
}

View* BoxLayout::LastVisibleView() const {
  for (int i = host_->child_count() - 1; i >= 0; --i) {
    View* result = host_->child_at(i);
    if (result->visible())
      return result;
  }
  return nullptr;
}

}  // namespace views
