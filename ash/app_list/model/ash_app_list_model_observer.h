// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_MODEL_ASH_APP_LIST_MODEL_OBSERVER_H_
#define ASH_APP_LIST_MODEL_ASH_APP_LIST_MODEL_OBSERVER_H_

#include <string>

#include "ash/app_list/model/app_list_model_export.h"
#include "base/macros.h"

namespace ui {
class MenuModel;
}  // namespace ui

// An interface to notify Chrome about changes or actions on AppListModel.
class APP_LIST_MODEL_EXPORT AshAppListModelObserver {
 public:
  virtual void ActivateItem(const std::string& id, int event_flags) {}
  virtual ui::MenuModel* GetContextMenuModel(const std::string& id) = 0;

 protected:
  AshAppListModelObserver() = default;
  virtual ~AshAppListModelObserver() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(AshAppListModelObserver);
};

#endif  // ASH_APP_LIST_MODEL_ASH_APP_LIST_MODEL_OBSERVER_H_
