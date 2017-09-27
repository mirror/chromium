// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_MODEL_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_MODEL_H_

#import <CoreGraphics/CoreGraphics.h>

class FullscreenController;

// Model object used to calculate fullscreen state.
class FullscreenModel {
 public:
  FullscreenModel(FullscreenController* controller);
  virtual ~FullscreenModel() = default;

  // The progress value calculated by the model.
  CGFloat progress() const { return progress_; }

  // Whether fullscreen is disabled.  When disabled, the toolbar is completely
  // visible.
  bool disabled() const { return disabled_counter_ > 0U; }

  // Whether the toolbar si fully visible or hidden.  The toolbar is fully
  // visible when progress() is 1.0, and fully hidden when progress() is 0.0.
  bool IsToolbarFullyVisible() const;
  bool IsToolbarFullyHidden() const;

  // Increments and decrements |disabled_counter_| for features that require the
  // toolbar be completely visible.
  void IncrementDisabledCounter();
  void DecrementDisabledCounter();

  // Recalculates the fullscreen progress for a new navigation.
  void ResetForNavigation();

  // Called when a scroll end animation finishes.  |progress| is the fullscreen
  // progress corresponding to the final state of the aniamtion.
  void AnimationEndedWithProgress(CGFloat progress);

  // Setter for the toolbar height to use in calculations.  Setting this resets
  // the model to a fully visible state.
  void SetToolbarHeight(CGFloat toolbar_height);

  // Setter for the current vertical content offset.  Setting this will
  // recalculate the progress value.
  void SetYContentOffset(CGFloat y_content_offset);

  // Setter for whether the scroll view is scrolling.  If a scroll event ends
  // and the progress value is not 0.0 or 1.0, the model will round to the
  // nearest value.
  void SetScrollViewIsScrolling(bool scrolling);

  // Setter for whether the scroll view is being dragged.  Unlocked base offsets
  // will be reset to all y content offset values received while the user is
  // not dragging.
  void SetScrollViewIsDragging(bool dragging);

  // Setter for whether the base content offset is locked.  If the base offset
  // is locked, the toolbar's location will be tied with a specific content
  // offset of the scroll view, rather than being able to be shown mid-way
  // through the page.
  void SetBaseOffsetLocked(bool locked);

 private:
  // Setter for |progress_|.  Notifies |controller_| of the new value if
  // |notify_observers| is true.
  void SetProgress(CGFloat progress, bool notify_observers = true);

  // Updates the base offset given the current y content offset, progress, and
  // toolbar height.
  void UpdateBaseOffset();

  // The FullscreenController that owns this model.  Used to notify observers of
  // model changes.
  FullscreenController* const controller_;
  // The percentage of the toolbar that should be visible, where 1.0 denotes a
  // fully visible toolbar and 0.0 denotes a completely hidden one.
  CGFloat progress_ = 0.0;
  // The base offset from which to calculate fullscreen state.  When |locked_|
  // is false, it is reset to the current offset after each scroll event.
  CGFloat base_offset_ = 0.0;
  // The height of the toolbar being shown or hidden by this model.
  CGFloat toolbar_height_ = 0.0;
  // The current vertical content offset of the main content.
  CGFloat y_content_offset_ = 0.0;
  // How many currently-running features require the toolbar be visible.
  size_t disabled_counter_ = 0;
  // Whether the main content is being scrolled.
  bool scrolling_ = false;
  // Whether the main content is being dragged.
  bool dragging_ = false;
  // Whether |base_offset_| is locked.
  bool locked_ = false;
  // Whether the next received y content offset value should be used as the new
  // base.
  bool update_base_offset_ = true;
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_MODEL_H_
