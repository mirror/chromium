// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_BAR_H_
#define CHROME_BROWSER_VR_ELEMENTS_BAR_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/vr/elements/textured_element.h"
#include "chrome/browser/vr/ui_unsupported_mode.h"
#include "components/security_state/core/security_state.h"
#include "url/gurl.h"

namespace vr {

class BarTexture;
// struct ToolbarState;

class Bar : public TexturedElement {
 public:
  Bar();
  ~Bar() override;

  void SetSize(float width, float height) override;
  void SetColor(SkColor color);

 private:
  UiTexture* GetTexture() const override;

  std::unique_ptr<BarTexture> texture_;

  DISALLOW_COPY_AND_ASSIGN(Bar);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_BAR_H_
