// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_ALERT_DIALOG_H_
#define CHROME_BROWSER_VR_ELEMENTS_ALERT_DIALOG_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/vr/elements/alert_dialog_texture.h"
#include "chrome/browser/vr/elements/textured_element.h"
#include "ui/gfx/vector_icon_types.h"

namespace vr {

class UiTexture;
// TODO This must use the texture that is provided to ui_scene_manager at this
// time. Title and Buttons are created here, but the texture should come fromt
// he VrShellDelegate

class AlertDialog : public TexturedElement {
 public:
  explicit AlertDialog(int preferred_width,
                       float heigh_meters,
                       const gfx::VectorIcon& icon,
                       base::string16 txt_message,
                       base::string16 second_message,
                       base::string16 buttons,
                       base::string16 buttons2);

  ~AlertDialog() override;
  void SetContent(long icon,
                  base::string16 title_text,
                  base::string16 toggle_text,
                  int b_positive,
                  base::string16 b_positive_text,
                  int b_negative,
                  base::string16 b_negative_text);

  void OnHoverEnter(const gfx::PointF& position) override;
  void OnHoverLeave() override;
  void OnMove(const gfx::PointF& position) override;

 protected:
  void UpdateElementSize() override;
  void OnStateUpdated(const gfx::PointF& position);

 private:
  UiTexture* GetTexture() const override;
  std::unique_ptr<AlertDialogTexture> texture_;
  float height_meters_;

  DISALLOW_COPY_AND_ASSIGN(AlertDialog);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_ALERT_DIALOG_H_
