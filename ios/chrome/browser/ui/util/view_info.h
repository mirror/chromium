// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_VIEW_INFO_H_
#define IOS_CHROME_BROWSER_UI_UTIL_VIEW_INFO_H_

#import <UIKit/UIKit.h>

// An object that exposes information about a specific view without giving
// access to the underlying view.  This class holds a weak reference to its
// underlying UIView, and should only be used when IsViewAlive() is true.
class ViewInfo {
 public:
  explicit ViewInfo(UIView* view);
  virtual ~ViewInfo();

  // Whether the view is alive.
  bool alive() const { return view_ != nil; }
  // The view's frame.
  CGRect frame() const { return view_.frame; }
  // The view's center.
  CGPoint center() const { return view_.center; }

  // Whether the view belongs to a window.
  bool HasWindow() const { return view_.window != nil; }

  // Returns the frame of the underlying view in |view|'s coordinate system.
  CGRect GetFrameInView(UIView* view) const;

  // Returns the center of the underlying view in window coordinates.
  CGPoint GetCenterInView(UIView* view) const;

 private:
  // The UIView passed on construction.
  __weak UIView* view_ = nil;
};

#endif  // IOS_CHROME_BROWSER_UI_UTIL_VIEW_INFO_H_
