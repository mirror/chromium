// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_TABSET_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_TABSET_MODEL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "third_party/skia/include/core/SkColor.h"

namespace vr {

// TODO(vollick): if this grows, it should move to its own header.
struct TabModel {
  TabModel(const base::string16& title, int id, SkColor color, int image_id);
  TabModel(const TabModel& other);
  ~TabModel();

  base::string16 title;
  int id = 0;
  SkColor color = SK_ColorWHITE;
  int image_id = -1;
};

struct TabSetModel {
  TabSetModel();
  TabSetModel(const TabSetModel& other);
  ~TabSetModel();

  bool incognito = false;
  // Eg, "last visited tabs", "incognito tabs", etc.
  base::string16 name;
  std::vector<TabModel> tabs;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_TABSET_MODEL_H_
