// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_VIEWS_BUBBLE_VIEW_H_
#define UI_CHROMEOS_KSV_VIEWS_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/view.h"

namespace gfx {
class ShadowValue;
struct VectorIcon;
}  // namespace gfx

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace keyboard_shortcut_viewer {

class BubbleView : public views::View {
 public:
  BubbleView();
  ~BubbleView() override;

  void SetIcon(const gfx::VectorIcon& icon);

  void SetText(const base::string16& text);

 private:
  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void OnPaint(gfx::Canvas* canvas) override;

  // Only one of the |icon_| or |text_| can be set.
  views::ImageView* icon_ = nullptr;
  views::Label* text_ = nullptr;

  std::vector<gfx::ShadowValue> shadows_;

  DISALLOW_COPY_AND_ASSIGN(BubbleView);
};

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_VIEWS_BUBBLE_VIEW_H_
