// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/tabset_model.h"

namespace vr {

TabModel::TabModel(const std::string& title, int id) : title(title), id(id) {}
TabModel::TabModel(const TabModel& other) = default;
TabModel::~TabModel() {}

TabSetModel::TabSetModel() {}
TabSetModel::TabSetModel(const TabSetModel& other) = default;
TabSetModel::~TabSetModel() {}

}  // namespace vr
