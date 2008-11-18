// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/browser/notifications/balloons.h"

#include "base/logging.h"
#include "base/gfx/rect.h"
#include "chrome/browser/notifications/balloon_view.h"
#include "chrome/browser/render_view_host.h"
#include "chrome/browser/render_widget_host_view_win.h"
#include "chrome/browser/site_instance.h"
#include "chrome/views/container_win.h"

namespace {
// Portion of the screen allotted for notifications.
// When notification balloons extend over this threshold, no new notifications
// are shown until some balloons are expired/closed.
const double kPercentBalloonFillFactor = 0.7;

// Allow at least this number of balloons on the screen.
const int kMinAllowedBalloonCount = 2;

void GetMainScreenWorkArea(gfx::Rect* bounds) {
  DCHECK(bounds);
  RECT work_area = {0};
  if (::SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0)) {
    bounds->SetRect(work_area.left,
                    work_area.top,
                    work_area.right,
                    work_area.bottom);
  } else {
    // If failed to call SystemParametersInfo for some reason, we simply get the
    // full screen size as an alternative.
    bounds->SetRect(0,
                    0,
                    ::GetSystemMetrics(SM_CXSCREEN) - 1,
                    ::GetSystemMetrics(SM_CYSCREEN) - 1);
  }
}

double GetSystemFontScaleFactor() {
  return 1.0;
}
}

BalloonCollection::BalloonCollection(BalloonCollectionObserver* observer)
    : observer_(observer) {
  DCHECK(observer);
  observer_->OnBalloonSpaceChanged();
}

bool BalloonCollection::HasSpace() const {
  if (count() < kMinAllowedBalloonCount) {
    return true;
  }

  int max_balloon_length = 0;
  int total_length = 0;
  layout_.GetMaxLengths(&max_balloon_length, &total_length);

  int current_max_length = max_balloon_length  * count();
  int max_allowed_length = static_cast<int>(total_length *
                                            kPercentBalloonFillFactor);
  return current_max_length < max_allowed_length - max_balloon_length;
}

void BalloonCollection::Add(const Notification& notification,
                            Profile* profile,
                            SiteInstance* site_instance) {
  Balloon* new_balloon = new Balloon(notification, profile, site_instance);
  gfx::Size size(layout_.min_balloon_width(), layout_.min_balloon_height());
  new_balloon->SetSize(size);
  balloons_.push_back(new_balloon);
  gfx::Point origin = layout_.GetOrigin();
  for (Balloons::iterator it = balloons_.begin(); it != balloons_.end(); ++it) {
    gfx::Point upper_left = layout_.NextPosition((*it)->size(), &origin);
    (*it)->SetPosition(upper_left);
  }
  new_balloon->Show();
  observer_->OnBalloonSpaceChanged();
}

void BalloonCollection::ShowAll() {
}

void BalloonCollection::HideAll() {
}

Balloon::Balloon(const Notification& notification,
                 Profile* profile,
                 SiteInstance* site_instance)
    : profile_(profile),
      site_instance_(site_instance),
      notification_(notification),
      state_(OPENING_BALLOON) {
}

Balloon::~Balloon() {
}

void Balloon::Show() {
  if (!balloon_view_.get()) {
    balloon_view_.reset(new BalloonView());
    balloon_view_->Show(this);
  }
}

void Balloon::SetPosition(const gfx::Point& upper_left) {
  position_ = upper_left;
}

void Balloon::SetSize(const gfx::Size& size) {
  size_ = size;
}

const gfx::Point& Balloon::position() const {
  return position_;
}

const gfx::Size& Balloon::size() const {
  return size_;
}

BalloonCollection::Layout::Placement BalloonCollection::Layout::placement_ =
    BalloonCollection::Layout::VERTICALLY_FROM_BOTTOM_RIGHT;

BalloonCollection::Layout::Layout()
    : font_scale_factor_(1.0) {
  RefreshSystemMetrics();
}

// Scale the size to count in the system font factor
int BalloonCollection::Layout::ScaleSize(int size) const {
  return static_cast<int>(size * font_scale_factor_);
}

int BalloonCollection::Layout::min_balloon_width() const {
  return ScaleSize(kBalloonMinWidth);
}

int BalloonCollection::Layout::max_balloon_width() const {
  return ScaleSize(kBalloonMaxWidth);
}

int BalloonCollection::Layout::min_balloon_height() const {
  return ScaleSize(kBalloonMinHeight);
}

int BalloonCollection::Layout::max_balloon_height() const {
  return ScaleSize(kBalloonMaxHeight);
}

gfx::Point BalloonCollection::Layout::GetOrigin() const {
  int x = 0;
  int y = 0;
  switch (placement_) {
    case HORIZONTALLY_FROM_BOTTOM_LEFT:
      x = work_area_.x();
      y = work_area_.bottom();
      break;
    case HORIZONTALLY_FROM_BOTTOM_RIGHT:
      x = work_area_.right();
      y = work_area_.bottom();
      break;
    case VERTICALLY_FROM_TOP_RIGHT:
      x = work_area_.right();
      y = work_area_.y();
      break;
    case VERTICALLY_FROM_BOTTOM_RIGHT:
      x = work_area_.right();
      y = work_area_.bottom();
      break;
    default:
      NOTREACHED();
      break;
  }
  return gfx::Point(x, y);
}

gfx::Point BalloonCollection::Layout::NextPosition(
    const gfx::Size& balloon_size,
    gfx::Point* origin) const {
  DCHECK(origin);

  int x = 0;
  int y = 0;
  switch (placement_) {
    case HORIZONTALLY_FROM_BOTTOM_LEFT:
      x = origin->x();
      y = origin->y() - balloon_size.height();
      origin->set_x(origin->x() + balloon_size.width());
      break;
    case HORIZONTALLY_FROM_BOTTOM_RIGHT:
      origin->set_x(origin->x() - balloon_size.width());
      x = origin->x();
      y = origin->y() - balloon_size.height();
      break;
    case VERTICALLY_FROM_TOP_RIGHT:
      x = origin->x() - balloon_size.width();
      y = origin->y();
      origin->set_y(origin->y() + balloon_size.height());
      break;
    case VERTICALLY_FROM_BOTTOM_RIGHT:
      origin->set_y(origin->y() - balloon_size.height());
      x = origin->x() - balloon_size.width();
      y = origin->y();
      break;
    default:
      NOTREACHED();
      break;
  }
  return gfx::Point(x, y);
}

bool BalloonCollection::Layout::RefreshSystemMetrics() {
  bool changed = false;

  gfx::Rect new_work_area(work_area_.x(), work_area_.y(),
                          work_area_.width(), work_area_.height());
  GetMainScreenWorkArea(&new_work_area);
  if (!work_area_.Equals(new_work_area)) {
    work_area_.SetRect(new_work_area.x(), new_work_area.y(),
                       new_work_area.width(), new_work_area.height());
    changed = true;
  }

  double new_font_scale_factor = font_scale_factor_;
  new_font_scale_factor = GetSystemFontScaleFactor();
  if (font_scale_factor_ != new_font_scale_factor) {
    font_scale_factor_ = new_font_scale_factor;
    changed = true;
  }
  return changed;
}

const void BalloonCollection::Layout::GetMaxLengths(
    int* max_balloon_length,
    int* total_length) const {
  DCHECK(max_balloon_length && total_length);

  switch (placement_) {
    case HORIZONTALLY_FROM_BOTTOM_LEFT:
    case HORIZONTALLY_FROM_BOTTOM_RIGHT:
      *total_length = work_area_.width();
      *max_balloon_length = max_balloon_width();
      break;
    case VERTICALLY_FROM_TOP_RIGHT:
    case VERTICALLY_FROM_BOTTOM_RIGHT:
      *total_length = work_area_.height();
      *max_balloon_length = max_balloon_height();
      break;
    default:
      NOTREACHED();
      break;
  }
}

#endif  // ENABLE_BACKGROUND_TASK
