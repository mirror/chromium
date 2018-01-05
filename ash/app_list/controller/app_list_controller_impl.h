// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_CONTROLLER_APP_LIST_CONTROLLER_IMPL_H_
#define ASH_APP_LIST_CONTROLLER_APP_LIST_CONTROLLER_IMPL_H_

#include <string>

#include "ash/app_list/controller/app_list_controller_export.h"
#include "ash/public/interfaces/app_list.mojom.h"
#include "base/macros.h"

namespace app_list {
class AppListModel;
class SearchModel;
}  // namespace app_list

namespace ash {

// This class implements methods to update AppListModel, SearchModel and
// SpeechUIModel from Chrome.
// TODO(hejq): Refactoring is still in process before we break app list to
//             two threads. We're pretending to implement a controller-client
//             pattern as the final state so that we can start the mojo
//             interface work. Once the Ash-Chrome breakage is done, change
//             |AppListControllerImpl| and |AppListControllerClientImpl| to
//             apply real mojo binding.
class APP_LIST_CONTROLLER_EXPORT AppListControllerImpl
    : public mojom::AppListController {
 public:
  AppListControllerImpl(app_list::AppListModel* app_list_model,
                        app_list::SearchModel* search_model);
  ~AppListControllerImpl() override;

  // Returns the single AppListControllerImpl instance.
  static AppListControllerImpl* Get() { return instance_; }

  // Wrappers around the mojom::AppListControllerClient interface.
  void ActivateItem(const std::string& id, int event_flags);

  // mojom::AppListcontroller:
  void SetClient(
      mojom::AppListControllerClientAssociatedPtrInfo client) override {}

  // Fake
  void SetClient(mojom::AppListControllerClient* client);

 private:
  static AppListControllerImpl* instance_;

  // Client interface in chrome browser.
  mojom::AppListControllerClient* app_list_client_ = nullptr;

  app_list::AppListModel* app_list_model_ = nullptr;
  app_list::SearchModel* search_model_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppListControllerImpl);
};

}  // namespace ash

#endif  // ASH_APP_LIST_CONTROLLER_APP_LIST_CONTROLLER_IMPL_H_
