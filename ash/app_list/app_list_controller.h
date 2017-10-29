// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_APP_LIST_CONTROLLER_H_
#define ASH_APP_LIST_APP_LIST_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/app_list.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace ash {

// Ash's ShelfController owns the ShelfModel and implements interface functions
// that allow Chrome to modify and observe the Shelf and ShelfModel state.
class ASH_EXPORT AppListController : public mojom::AppListController {
 public:
  AppListController();
  ~AppListController() override;

  // Binds the mojom::ShelfController interface request to this object.
  void BindRequest(mojom::AppListControllerRequest request);

  // mojom::AppListController:
  void SetObserver(mojom::AppListObserverAssociatedPtrInfo observer) override;
  void DeleteItem(const std::string& id) override;

  // Bindings for the ShelfController interface.
  mojo::BindingSet<mojom::AppListController> bindings_;

  // The set of shelf observers notified about state and model changes.
  mojo::AssociatedInterfacePtr<mojom::AppListObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(AppListController);
};

}  // namespace ash

#endif  // ASH_APP_LIST_APP_LIST_CONTROLLER_H_
