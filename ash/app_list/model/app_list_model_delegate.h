// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_MODEL_APP_LIST_MODEL_DELEGATE_H_
#define ASH_APP_LIST_MODEL_APP_LIST_MODEL_DELEGATE_H_

#include "ash/app_list/model/app_list_model_export.h"
#include "ash/public/interfaces/app_list.mojom.h"

namespace app_list {

class APP_LIST_MODEL_EXPORT AppListModelDelegate {
 public:
  // Triggered after an item has been added to the model.
  virtual void OnAppListItemAdded(
      ash::mojom::AppListItemMetadataPtr item_data) {}

  // Triggered just before an item is deleted from the model.
  virtual void OnAppListItemWillBeDeleted(
      ash::mojom::AppListItemMetadataPtr item_data) {}

  // Triggered after an item has moved, changed folders, or changed properties.
  virtual void OnAppListItemUpdated(
      ash::mojom::AppListItemMetadataPtr item_data) {}

 protected:
  virtual ~AppListModelDelegate() {}
};

}  // namespace app_list

#endif  // ASH_APP_LIST_MODEL_APP_LIST_MODEL_DELEGATE_H_
