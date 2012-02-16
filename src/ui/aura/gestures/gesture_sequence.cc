// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/gestures/gesture_sequence.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "ui/aura/event.h"
#include "ui/base/events.h"

// TODO(sad): Pinch gestures currently always assume that the first two
//            touch-points (i.e. at indices 0 and 1) are involved. This may not
//            always be the case. This needs to be fixed eventually.
//            http://crbug.com/113144

namespace {

// TODO(girard): Make these configurable in sync with this CL
//               http://crbug.com/100773
const float kMinimumPinchUpdateDistance = 5;  // in pixels
const float kMinimumDistanceForPinchScroll = 20;

}  // namespace

namespace aura {

namespace {

// ui::EventType is mapped to TouchState so it can fit into 3 bits of
// Signature.
enum TouchState {
  TS_RELEASED,
  TS_PRESSED,
  TS_MOVED,
  TS_STATIONARY,
  TS_CANCELLED,
  TS_UNKNOWN,
};

// Get equivalent TouchState from EventType |type|.
TouchState TouchEventTypeToTouchState(ui::EventType type) {
  switch (type) {
    case ui::ET_TOUCH_RELEASED:
      return TS_RELEASED;
    case ui::ET_TOUCH_PRESSED:
      return TS_PRESSED;
    case ui::ET_TOUCH_MOVED:
      return TS_MOVED;
    case ui::ET_TOUCH_STATIONARY:
      return TS_STATIONARY;
    case ui::ET_TOUCH_CANCELLED:
      return TS_CANCELLED;
    default:
      VLOG(1) << "Unknown Touch Event type";
  }
  return TS_UNKNOWN;
}

// Gesture signature types for different values of combination (GestureState,
// touch_id, ui::EventType, touch_handled), see Signature for more info.
//
// Note: New addition of types should be placed as per their Signature value.
#define G(gesture_state, id, touch_state, handled) 1 + ( \
  (((touch_state) & 0x7) << 1) |                         \
  ((handled) ? (1 << 4) : 0) |                           \
  (((id) & 0xfff) << 5) |                                \
  ((gesture_state) << 17))

enum EdgeStateSignatureType {
  GST_NO_GESTURE_FIRST_PRESSED =
      G(GS_NO_GESTURE, 0, TS_PRESSED, false),

  GST_PENDING_SYNTHETIC_CLICK_FIRST_RELEASED =
      G(GS_PENDING_SYNTHETIC_CLICK, 0, TS_RELEASED, false),

  GST_PENDING_SYNTHETIC_CLICK_FIRST_MOVED =
      G(GS_PENDING_SYNTHETIC_CLICK, 0, TS_MOVED, false),

  GST_PENDING_SYNTHETIC_CLICK_FIRST_STATIONARY =
      G(GS_PENDING_SYNTHETIC_CLICK, 0, TS_STATIONARY, false),

  GST_PENDING_SYNTHETIC_CLICK_FIRST_CANCELLED =
      G(GS_PENDING_SYNTHETIC_CLICK, 0, TS_CANCELLED, false),

  GST_PENDING_SYNTHETIC_CLICK_SECOND_PRESSED =
      G(GS_PENDING_SYNTHETIC_CLICK, 1, TS_PRESSED, false),

  GST_SCROLL_FIRST_RELEASED =
      G(GS_SCROLL, 0, TS_RELEASED, false),

  GST_SCROLL_FIRST_MOVED =
      G(GS_SCROLL, 0, TS_MOVED, false),

  GST_SCROLL_FIRST_CANCELLED =
      G(GS_SCROLL, 0, TS_CANCELLED, false),

  GST_SCROLL_FIRST_PRESSED =
      G(GS_SCROLL, 0, TS_PRESSED, false),

  GST_SCROLL_SECOND_RELEASED =
      G(GS_SCROLL, 1, TS_RELEASED, false),

  GST_SCROLL_SECOND_MOVED =
      G(GS_SCROLL, 1, TS_MOVED, false),

  GST_SCROLL_SECOND_CANCELLED =
      G(GS_SCROLL, 1, TS_CANCELLED, false),

  GST_SCROLL_SECOND_PRESSED =
      G(GS_SCROLL, 1, TS_PRESSED, false),

  GST_PINCH_FIRST_MOVED =
      G(GS_PINCH, 0, TS_MOVED, false),

  GST_PINCH_SECOND_MOVED =
      G(GS_PINCH, 1, TS_MOVED, false),

  GST_PINCH_FIRST_RELEASED =
      G(GS_PINCH, 0, TS_RELEASED, false),

  GST_PINCH_SECOND_RELEASED =
      G(GS_PINCH, 1, TS_RELEASED, false),

  GST_PINCH_FIRST_CANCELLED =
      G(GS_PINCH, 0, TS_CANCELLED, false),

  GST_PINCH_SECOND_CANCELLED =
      G(GS_PINCH, 1, TS_CANCELLED, false),
};

// Builds a signature. Signatures are assembled by joining together
// multiple bits.
// 1 LSB bit so that the computed signature is always greater than 0
// 3 bits for the |type|.
// 1 bit for |touch_handled|
// 12 bits for |touch_id|
// 15 bits for the |gesture_state|.
EdgeStateSignatureType Signature(GestureState gesture_state,
                                 unsigned int touch_id,
                                 ui::EventType type,
                                 bool touch_handled) {
  CHECK((touch_id & 0xfff) == touch_id);
  TouchState touch_state = TouchEventTypeToTouchState(type);
  return static_cast<EdgeStateSignatureType>
      (G(gesture_state, touch_id, touch_state, touch_handled));
}
#undef G

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// GestureSequence Public:

GestureSequence::GestureSequence()
    : state_(GS_NO_GESTURE),
      flags_(0),
      pinch_distance_start_(0.f),
      pinch_distance_current_(0.f),
      point_count_(0) {
}

GestureSequence::~GestureSequence() {
}

GestureSequence::Gestures* GestureSequence::ProcessTouchEventForGesture(
    const TouchEvent& event,
    ui::TouchStatus status) {
  if (status != ui::TOUCH_STATUS_UNKNOWN)
    return NULL;  // The event was consumed by a touch sequence.

  // Set a limit on the number of simultaneous touches in a gesture.
  if (event.touch_id() >= kMaxGesturePoints)
    return NULL;

  if (event.type() == ui::ET_TOUCH_PRESSED) {
    if (point_count_ == kMaxGesturePoints)
      return NULL;
    ++point_count_;
  }

  GestureState last_state = state_;

  // NOTE: when modifying these state transitions, also update gestures.dot
  scoped_ptr<Gestures> gestures(new Gestures());
  GesturePoint& point = GesturePointForEvent(event);
  point.UpdateValues(event);
  flags_ = event.flags();
  switch (Signature(state_, event.touch_id(), event.type(), false)) {
    case GST_NO_GESTURE_FIRST_PRESSED:
      TouchDown(event, point, gestures.get());
      set_state(GS_PENDING_SYNTHETIC_CLICK);
      break;
    case GST_PENDING_SYNTHETIC_CLICK_FIRST_RELEASED:
      if (Click(event, point, gestures.get()))
        point.UpdateForTap();
      set_state(GS_NO_GESTURE);
      break;
    case GST_PENDING_SYNTHETIC_CLICK_FIRST_MOVED:
    case GST_PENDING_SYNTHETIC_CLICK_FIRST_STATIONARY:
      if (ScrollStart(event, point, gestures.get())) {
        set_state(GS_SCROLL);
        if (ScrollUpdate(event, point, gestures.get()))
          point.UpdateForScroll();
      }
      break;
    case GST_PENDING_SYNTHETIC_CLICK_FIRST_CANCELLED:
      NoGesture(event, point, gestures.get());
      break;
    case GST_SCROLL_FIRST_MOVED:
    case GST_SCROLL_SECOND_MOVED:
      if (scroll_type_ == ST_VERTICAL ||
          scroll_type_ == ST_HORIZONTAL)
        BreakRailScroll(event, point, gestures.get());
      if (ScrollUpdate(event, point, gestures.get()))
        point.UpdateForScroll();
      break;
    case GST_SCROLL_FIRST_RELEASED:
    case GST_SCROLL_FIRST_CANCELLED:
    case GST_SCROLL_SECOND_RELEASED:
    case GST_SCROLL_SECOND_CANCELLED:
      ScrollEnd(event, point, gestures.get());
      set_state(GS_NO_GESTURE);
      break;
    case GST_SCROLL_FIRST_PRESSED:
    case GST_SCROLL_SECOND_PRESSED:
    case GST_PENDING_SYNTHETIC_CLICK_SECOND_PRESSED:
      PinchStart(event, point, gestures.get());
      set_state(GS_PINCH);
      break;
    case GST_PINCH_FIRST_MOVED:
    case GST_PINCH_SECOND_MOVED:
      if (PinchUpdate(event, point, gestures.get())) {
        points_[0].UpdateForScroll();
        points_[1].UpdateForScroll();
      }
      break;
    case GST_PINCH_FIRST_RELEASED:
    case GST_PINCH_SECOND_RELEASED:
    case GST_PINCH_FIRST_CANCELLED:
    case GST_PINCH_SECOND_CANCELLED:
      PinchEnd(event, point, gestures.get());

      // Once pinch ends, it should still be possible to scroll with the
      // remaining finger on the screen.
      scroll_type_ = ST_FREE;
      set_state(GS_SCROLL);
      break;
  }

  if (state_ != last_state)
    VLOG(4) << "Gesture Sequence"
            << " State: " << state_
            << " touch id: " << event.touch_id();

  if (event.type() == ui::ET_TOUCH_RELEASED)
    --point_count_;

  return gestures.release();
}

void GestureSequence::Reset() {
  set_state(GS_NO_GESTURE);
  for (int i = 0; i < point_count_; ++i)
    points_[i].Reset();
}

////////////////////////////////////////////////////////////////////////////////
// GestureSequence Private:

GesturePoint& GestureSequence::GesturePointForEvent(
    const TouchEvent& event) {
  return points_[event.touch_id()];
}

void GestureSequence::AppendTapDownGestureEvent(const GesturePoint& point,
                                                Gestures* gestures) {
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_TAP_DOWN,
      point.first_touch_position().x(),
      point.first_touch_position().y(),
      flags_,
      base::Time::FromDoubleT(point.last_touch_time()),
      0.f, 0.f)));
}

void GestureSequence::AppendClickGestureEvent(const GesturePoint& point,
                                              Gestures* gestures) {
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_TAP,
      point.first_touch_position().x(),
      point.first_touch_position().y(),
      flags_,
      base::Time::FromDoubleT(point.last_touch_time()),
      0.f, 0.f)));
}

void GestureSequence::AppendDoubleClickGestureEvent(const GesturePoint& point,
                                                    Gestures* gestures) {
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_DOUBLE_TAP,
      point.first_touch_position().x(),
      point.first_touch_position().y(),
      flags_,
      base::Time::FromDoubleT(point.last_touch_time()),
      0.f, 0.f)));
}

void GestureSequence::AppendScrollGestureBegin(const GesturePoint& point,
                                               const gfx::Point& location,
                                               Gestures* gestures) {
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_SCROLL_BEGIN,
      location.x(),
      location.y(),
      flags_,
      base::Time::FromDoubleT(point.last_touch_time()),
      0.f, 0.f)));
}

void GestureSequence::AppendScrollGestureEnd(const GesturePoint& point,
                                             const gfx::Point& location,
                                             Gestures* gestures,
                                             float x_velocity,
                                             float y_velocity) {
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_SCROLL_END,
      location.x(),
      location.y(),
      flags_,
      base::Time::FromDoubleT(point.last_touch_time()),
      x_velocity, y_velocity)));
}

void GestureSequence::AppendScrollGestureUpdate(const GesturePoint& point,
                                                const gfx::Point& location,
                                                Gestures* gestures) {
  int dx = point.x_delta();
  int dy = point.y_delta();

  if (scroll_type_ == ST_HORIZONTAL)
    dy = 0;
  else if (scroll_type_ == ST_VERTICAL)
    dx = 0;

  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_SCROLL_UPDATE,
      location.x(),
      location.y(),
      flags_,
      base::Time::FromDoubleT(point.last_touch_time()),
      dx, dy)));
}

void GestureSequence::AppendPinchGestureBegin(const GesturePoint& p1,
                                              const GesturePoint& p2,
                                              Gestures* gestures) {
  gfx::Point center = p1.last_touch_position().Middle(p2.last_touch_position());
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_PINCH_BEGIN,
      center.x(),
      center.y(),
      flags_,
      base::Time::FromDoubleT(p1.last_touch_time()),
      0.f, 0.f)));
}

void GestureSequence::AppendPinchGestureEnd(const GesturePoint& p1,
                                            const GesturePoint& p2,
                                            float scale,
                                            Gestures* gestures) {
  gfx::Point center = p1.last_touch_position().Middle(p2.last_touch_position());
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_PINCH_END,
      center.x(),
      center.y(),
      flags_,
      base::Time::FromDoubleT(p1.last_touch_time()),
      scale, 0.f)));
}

void GestureSequence::AppendPinchGestureUpdate(const GesturePoint& p1,
                                               const GesturePoint& p2,
                                               float scale,
                                               Gestures* gestures) {
  // TODO(sad): Compute rotation and include it in delta_y.
  // http://crbug.com/113145
  gfx::Point center = p1.last_touch_position().Middle(p2.last_touch_position());
  gestures->push_back(linked_ptr<GestureEvent>(new GestureEvent(
      ui::ET_GESTURE_PINCH_UPDATE,
      center.x(),
      center.y(),
      flags_,
      base::Time::FromDoubleT(p1.last_touch_time()),
      scale, 0.f)));
}

bool GestureSequence::Click(const TouchEvent& event,
    const GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_PENDING_SYNTHETIC_CLICK);
  if (point.IsInClickWindow(event)) {
    AppendClickGestureEvent(point, gestures);
    if (point.IsInDoubleClickWindow(event))
      AppendDoubleClickGestureEvent(point, gestures);
    return true;
  }
  return false;
}

bool GestureSequence::ScrollStart(const TouchEvent& event,
    GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_PENDING_SYNTHETIC_CLICK);
  if (point.IsInClickWindow(event) ||
      !point.IsInScrollWindow(event) ||
      !point.HasEnoughDataToEstablishRail())
    return false;
  AppendScrollGestureBegin(point, point.last_touch_position(), gestures);
  if (point.IsInHorizontalRailWindow())
    scroll_type_ = ST_HORIZONTAL;
  else if (point.IsInVerticalRailWindow())
    scroll_type_ = ST_VERTICAL;
  else
    scroll_type_ = ST_FREE;
  return true;
}

void GestureSequence::BreakRailScroll(const TouchEvent& event,
    GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_SCROLL);
  if (scroll_type_ == ST_HORIZONTAL &&
      point.BreaksHorizontalRail())
    scroll_type_ = ST_FREE;
  else if (scroll_type_ == ST_VERTICAL &&
           point.BreaksVerticalRail())
    scroll_type_ = ST_FREE;
}

bool GestureSequence::ScrollUpdate(const TouchEvent& event,
    const GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_SCROLL);
  if (!point.DidScroll(event, 0))
    return false;
  AppendScrollGestureUpdate(point, point.last_touch_position(), gestures);
  return true;
}

bool GestureSequence::NoGesture(const TouchEvent&,
    const GesturePoint& point, Gestures*) {
  Reset();
  return false;
}

bool GestureSequence::TouchDown(const TouchEvent& event,
    const GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_NO_GESTURE);
  AppendTapDownGestureEvent(point, gestures);
  return true;
}

bool GestureSequence::ScrollEnd(const TouchEvent& event,
    GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_SCROLL);
  if (point.IsInFlickWindow(event)) {
    AppendScrollGestureEnd(point, point.last_touch_position(), gestures,
        point.XVelocity(), point.YVelocity());
  } else {
    AppendScrollGestureEnd(point, point.last_touch_position(), gestures,
        0.f, 0.f);
  }
  return true;
}

bool GestureSequence::PinchStart(const TouchEvent& event,
    const GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_SCROLL ||
         state_ == GS_PENDING_SYNTHETIC_CLICK);
  AppendTapDownGestureEvent(point, gestures);

  pinch_distance_current_ = points_[0].Distance(points_[1]);
  pinch_distance_start_ = pinch_distance_current_;
  AppendPinchGestureBegin(points_[0], points_[1], gestures);

  if (state_ == GS_PENDING_SYNTHETIC_CLICK) {
    gfx::Point center = points_[0].last_touch_position().Middle(
        points_[1].last_touch_position());
    AppendScrollGestureBegin(point, center, gestures);
  }

  return true;
}

bool GestureSequence::PinchUpdate(const TouchEvent& event,
    const GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_PINCH);
  float distance = points_[0].Distance(points_[1]);
  if (abs(distance - pinch_distance_current_) < kMinimumPinchUpdateDistance) {
    // The fingers didn't move towards each other, or away from each other,
    // enough to constitute a pinch. But perhaps they moved enough in the same
    // direction to do a two-finger scroll.
    if (!points_[0].DidScroll(event, kMinimumDistanceForPinchScroll) ||
        !points_[1].DidScroll(event, kMinimumDistanceForPinchScroll))
      return false;

    gfx::Point center = points_[0].last_touch_position().Middle(
                        points_[1].last_touch_position());
    AppendScrollGestureUpdate(point, center, gestures);
  } else {
    AppendPinchGestureUpdate(points_[0], points_[1],
        distance / pinch_distance_current_, gestures);
    pinch_distance_current_ = distance;
  }
  return true;
}

bool GestureSequence::PinchEnd(const TouchEvent& event,
    const GesturePoint& point, Gestures* gestures) {
  DCHECK(state_ == GS_PINCH);
  float distance = points_[0].Distance(points_[1]);
  AppendPinchGestureEnd(points_[0], points_[1],
      distance / pinch_distance_start_, gestures);

  pinch_distance_start_ = 0;
  pinch_distance_current_ = 0;
  return true;
}

}  // namespace aura
