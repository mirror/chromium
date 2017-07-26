// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_LOCATION_BAR_H_
#define CHROME_BROWSER_VR_ELEMENTS_LOCATION_BAR_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"

namespace vr {

class LocationBar : public UiElement {
 public:
  LocationBar(UiElement* background_element, UiElement* thumb_element);
  ~LocationBar() override;

  void SetSize(float width, float hight) override;

  void SetRange(size_t range);
  void SetPosition(size_t position);
  size_t GetRange() const;
  size_t GetPosition() const;

 private:
  void LayOutChildren() override;
  UiElement* background_element_ = nullptr;
  UiElement* thumb_element_ = nullptr;
  size_t range_ = 1;
  size_t position_ = 0;

  DISALLOW_COPY_AND_ASSIGN(LocationBar);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_LOCATION_BAR_H_
