// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Handles the visible notification (or balloons).

#ifndef CHROME_BROWSER_NOTIFICATIONS_BALLOONS_H_
#define CHROME_BROWSER_NOTIFICATIONS_BALLOONS_H_

#ifdef ENABLE_BACKGROUND_TASK

#include <deque>

#include "base/basictypes.h"
#include "base/gfx/point.h"
#include "base/gfx/rect.h"
#include "base/gfx/size.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/notifications/notification.h"

class Balloon;
class BalloonView;
class Profile;
class SiteInstance;

class BalloonCollectionObserver {
 public:
  // Called when there is more or less space for balloons due to
  // monitor size changes or balloons disappearing.
  virtual void OnBalloonSpaceChanged() = 0;
};

class BalloonCollectionInterface {
 public:
  virtual ~BalloonCollectionInterface() {}

  // Adds a new balloon for the specified notification.
  virtual void Add(const Notification& notification,
                   Profile* profile,
                   SiteInstance* site_instance) = 0;

  // Show all balloons.
  virtual void ShowAll() = 0;

  // Hide all balloons.
  virtual void HideAll() = 0;

  // Is there room to add another notification?
  virtual bool HasSpace() const = 0;

  // Number of balloons being shown.
  virtual int count() const = 0;
};

typedef std::deque<Balloon*> Balloons;

class BalloonCollection : public BalloonCollectionInterface {
 public:
  explicit BalloonCollection(BalloonCollectionObserver* observer);

  // BalloonCollectionInterface overrides
  virtual void Add(const Notification& notification,
                   Profile* profile,
                   SiteInstance* site_instance);
  virtual void ShowAll();
  virtual void HideAll();
  virtual bool HasSpace() const;
  virtual int count() const { return 0; }

 private:
  // Calculates layout values for the balloons including
  // the scaling, the max/min sizes, and the upper left corner of each.
  class Layout {
   public:
    Layout();

    // Refresh the work area and balloon placement.
    void OnDisplaySettingsChanged();

    int min_balloon_width() const;
    int max_balloon_width() const;
    int min_balloon_height() const;
    int max_balloon_height() const;

    // Returns both the total lenght available and the maximum
    // allowed per balloon.
    //
    // The length may be a height or length depending on the way that
    // balloons are laid out.
    const void GetMaxLengths(int* max_balloon_length, int* total_length) const;

    // Scale the size to count in the system font factor.
    int ScaleSize(int size) const;

    // Refresh the cached values for work area and drawing metrics.
    // This is done automatically first time and the application should
    // call this method to re-acquire metrics after any
    // resolution or settings change.
    //
    // Return true if and only if a metric changed.
    bool RefreshSystemMetrics();

    // Returns the starting value for NextUpperLeftPosition.
    gfx::Point GetOrigin() const;

    // Compute the position for the next balloon.
    //   Modifies origin.
    //   Returns the position of the upper left coordinate for the given
    //   balloon.
    gfx::Point NextPosition(const gfx::Size& balloon_size,
                            gfx::Point* origin) const;

   private:
    enum Placement {
      HORIZONTALLY_FROM_BOTTOM_LEFT,
      HORIZONTALLY_FROM_BOTTOM_RIGHT,
      VERTICALLY_FROM_TOP_RIGHT,
      VERTICALLY_FROM_BOTTOM_RIGHT
    };

    // Minimum and maximum size of balloon
    const static int kBalloonMinWidth = 300;
    const static int kBalloonMaxWidth = 300;
    const static int kBalloonMinHeight = 70;
    const static int kBalloonMaxHeight = 120;

    static Placement placement_;
    gfx::Rect work_area_;
    double font_scale_factor_;
    DISALLOW_COPY_AND_ASSIGN(Layout);
  };

  BalloonCollectionObserver* observer_;
  Balloons balloons_;
  Layout layout_;
  DISALLOW_COPY_AND_ASSIGN(BalloonCollection);
};

// Represents a Notification on the screen.
class Balloon {
 public:
  Balloon(const Notification& notification,
          Profile* profile,
          SiteInstance* site_instance);
  ~Balloon();

  const Notification& notification() const {
    return notification_;
  }

  Profile* profile() const {
    return profile_;
  }

  SiteInstance* site_instance() const {
    return site_instance_;
  }

  void SetPosition(const gfx::Point& upper_left);
  void SetSize(const gfx::Size& size);
  void Show();
  const gfx::Point& position() const;
  const gfx::Size& size() const;

 private:
  enum BalloonState {
    OPENING_BALLOON,
    SHOWING_BALLOON,
    AUTO_CLOSING_BALLOON,
    USER_CLOSING_BALLOON,
    RESTORING_BALLOON,
  };

  Profile* profile_;
  SiteInstance* site_instance_;
  Notification notification_;
  BalloonState state_;
  scoped_ptr<BalloonView> balloon_view_;
  gfx::Point position_;
  gfx::Size size_;
  DISALLOW_COPY_AND_ASSIGN(Balloon);
};

#endif  // ENABLE_BACKGROUND_TASK

#endif  // CHROME_BROWSER_NOTIFICATIONS_BALLOONS_H_
