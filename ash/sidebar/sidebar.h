// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SIDEBAR_SIDEBAR_H_
#define ASH_SIDEBAR_SIDEBAR_H_

#include <memory>

#include "ash/sidebar/sidebar_mode.h"

namespace aura {
class Window;
}

namespace ash {

class Shelf;
class SidebarWidget;

class Sidebar {
 public:
  Sidebar();
  virtual ~Sidebar();

  // Returns a Sidebar of the window's root window.
  static Sidebar* ForWindow(const aura::Window* window);

  void SetRootAndShelf(aura::Window* root, Shelf* shelf);
  void Show(SidebarMode mode);
  void Hide();
  bool IsVisible() const;

 private:
  SidebarWidget* widget_ = nullptr;
  aura::Window* root_ = nullptr;
  Shelf* shelf_ = nullptr;
};

}  // namespace ash

#endif  // ASH_SIDEBAR_SIDEBAR_H_
