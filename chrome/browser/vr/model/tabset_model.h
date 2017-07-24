// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_TABSET_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_TABSET_MODEL_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace vr {

// TODO(vollick): if this grows, it should move to its own header.
struct TabModel {
  TabModel(const std::string& title, int id);
  TabModel(const TabModel& other);
  ~TabModel();

  std::string title;
  int id = 0;
};

struct TabSetModel {
  TabSetModel();
  TabSetModel(const TabSetModel& other);
  ~TabSetModel();

  bool incognito = false;
  // Eg, "last visited tabs", "incognito tabs", etc.
  std::string name;
  std::vector<TabModel> tabs;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_TABSET_MODEL_H_
